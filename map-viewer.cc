#include "graphics.h"
#include "led-matrix.h"
#include "city.h"

#include <Eigen/Dense>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

vector<string> tokenize_csv_line(string line);
static void interrupt_handler(int signal);
static void print_usage(const char *prog_name);
static void transform_coords(vector<City> *all_cities, vector<City> *ref_cities);
static vector<City> get_all_cities();
static vector<City> get_ref_cities(vector<City> *all_cities, string ref_string);

volatile bool interrupt_received = false;

int main(int argc, char *argv[])  {

    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv);
    if (matrix == NULL)
        return 1;

    // Parse commmad line arguments.
    string ref_cities_string;
    int opt;
    while ((opt = getopt(argc, argv, "r:")) != -1) {
        switch (opt) {
            case 'r':
                ref_cities_string = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    cout << "[1/5] Creating all cities. This might take a minute." << endl;
    vector<City> all_cities = get_all_cities();

    cout << "[2/5] Creating reference cities." << endl;
    vector<City> ref_cities = get_ref_cities(&all_cities, ref_cities_string);

    cout << "[3/5] Transforming coordinates." << endl;
    transform_coords(&all_cities, &ref_cities);

    cout << "[4/5] Reading states stats." << endl;

    string line;
    ifstream line_stream("daily.csv");
    if (!line_stream.is_open()) {
        cerr << "Error opening states CSV." << endl;
        return 1;
    }

    const string DATE_SELECTION = "20200814";
    map<string, unsigned int> statePositive;
    unsigned int positive_min = LONG_MAX;
    unsigned int positive_max = 0;

    getline(line_stream, line); // Skip the first line with column names.

    while (getline(line_stream, line)) {
        vector<string> tokens = tokenize_csv_line(line);
        
        // Skip if no match.
        string date = tokens[0];
        if (DATE_SELECTION.compare(date))
            continue;
        
        string name = tokens[1];
        unsigned int positive = stol(tokens[2]);
        
        statePositive[name] = positive;

        positive_min = min(positive_min, positive);
        positive_max = max(positive_max, positive);
    }

    cout << "[5/5] Displaying map." << endl;
    for(const auto& city: all_cities) {

        unsigned char red_min = 1;
        unsigned char red_max = 255;

        long int positive = statePositive[city.state];
        
        // Remap number of positive from linear scale (positive_min .. positive_max)
        // to logarithmic scale (red_min .. red_max).
        float lin_min = positive_max;
        float lin_max = positive_min;
        float log_min = red_min;
        float log_max = red_max;
        float b = log(log_min / log_max) / (lin_min - lin_max);
        float a = log_min / exp(b * lin_min);
        int red = red_max - a * exp(b * positive);

        matrix->SetPixel(city.x, city.y, red, 0, 0);
    }

    // Display reference cities in a different color.
    for(const auto& city: ref_cities)
        matrix->SetPixel(city.x, city.y, COLOR_BLUE.r, COLOR_BLUE.g, COLOR_BLUE.b);

    signal(SIGINT, interrupt_handler);
    cout << "Done. Press Ctrl+C to exit." << endl;
    do {
        // Update here.
    } while (!interrupt_received);
    cout << endl;

    matrix->Clear();
}

vector<string> tokenize_csv_line(string line) {
    vector<string> tokens;
    stringstream line_stream(line);
    string token;
    while (getline(line_stream, token, ',')) {
        
        // Remove any surrounding quotation marks.
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        
        tokens.push_back(token);
    }
    return tokens;
}

static void print_usage(const char *prog_name) {
    cerr << "Usage: sudo " << prog_name << " [options]" << endl;
    cerr << "\tDisplay US cities using matrix coordinates of three cities as reference." << endl;
    cerr << "Options:" << endl;
    cerr << "\t-r <string> : Comma-separated reference string with format," << endl;
    cerr << "\t\t      \"<city1>,<state1>,<x1>,<y1>,<city2>,<state2>,<x2>,<y2>,<city3>,<state3>,<x3>,<y3>\"" << endl;
    cerr << "\t\t      (default=\"Seattle,WA,10,10,El Paso,TX,60,50,Portland,ME,110,10\")" << endl;
}

void transform_coords(vector<City> *all_cities, vector<City> *ref_cities) {

    // Find the transformation required to get reference cities' coordinates
    // from (<lng>, <lng>) to (<x>, <y>).
    Eigen::MatrixXf A(6, 6);
    A << 
        (*ref_cities)[0].lng, (*ref_cities)[0].lat, 1, 0, 0, 0,
        0, 0, 0, (*ref_cities)[0].lng, (*ref_cities)[0].lat, 1,
        (*ref_cities)[1].lng, (*ref_cities)[1].lat, 1, 0, 0, 0,
        0, 0, 0, (*ref_cities)[1].lng, (*ref_cities)[1].lat, 1,
        (*ref_cities)[2].lng, (*ref_cities)[2].lat, 1, 0, 0, 0,
        0, 0, 0, (*ref_cities)[2].lng, (*ref_cities)[2].lat, 1;
    Eigen::MatrixXf A_prime(6, 1);
    A_prime << (*ref_cities)[0].x, (*ref_cities)[0].y,
               (*ref_cities)[1].x, (*ref_cities)[1].y,
               (*ref_cities)[2].x, (*ref_cities)[2].y;
    Eigen::MatrixXf trans = A.inverse() * A_prime;

    vector<City>::iterator it;
    for(it = all_cities->begin(); it != all_cities->end(); it++) {

        // Transform spherical coordinates into planar coordinates.
        // The plate carrée projection simply maps x to be the value of the longitude 
        // and y to be the value of the latitude.
        float planar_x = (*it).lng;
        float planar_y = (*it).lat;
        
        // Transform planar coordinates into matrix coordinates.
        Eigen::MatrixXf B(2, 6);
        B <<
            planar_x, planar_y, 1, 0, 0, 0,
            0, 0, 0, planar_x, planar_y, 1;
        Eigen::MatrixXf B_prime = B * trans;
        (*it).x = int(B_prime(0, 0));
        (*it).y = int(B_prime(1, 0));
    }

}

vector<City> get_all_cities() {
    vector<City> cities;
    string line;
    ifstream line_stream("uscities.csv");
    if (!line_stream.is_open()) {
        cerr << "Error opening cities CSV." << endl;
    }

    // Skip the first line with column names.
    getline(line_stream, line);

    // For each record, create a new city and insert.
    while (getline(line_stream, line)) {
    
        vector<string> tokens = tokenize_csv_line(line);
        string name = tokens[0];
        string state = tokens[2];
        float lng = atof(tokens[9].c_str());
        float lat = atof(tokens[8].c_str());

        City new_city(name, state, lng, lat);
        cities.insert(cities.begin(), new_city);
    }
    return cities;
}

vector<City> get_ref_cities(vector<City> *all_cities, string ref_string) {
    vector<City> ref_cities;
    const int N_REF_CITIES = 3;

    vector<string> tokens = tokenize_csv_line(ref_string);

    for (int i = 0; i < N_REF_CITIES; i++) {
        int idx = i * (N_REF_CITIES + 1);

        // ref_string should follow the format, "<name1>,<state1>,<x1>,<y1>,..."
        string name = tokens[idx];
        string state = tokens[idx + 1];
        int x = atoi(tokens[idx + 2].c_str());
        int y = atoi(tokens[idx + 3].c_str());

        vector<City>::iterator it;
        for(it = all_cities->begin(); it != all_cities->end(); it++) {
            
            // Skip if no match.
            if ((*it).name.compare(name) || (*it).state.compare(state))
                continue;
            
            City new_city((*it).name, (*it).state, (*it).lng, (*it).lat, x, y);
            ref_cities.insert(ref_cities.begin(), new_city);
        }
    }

    return ref_cities;
}

static void interrupt_handler(int signal) {
    interrupt_received = true;
}