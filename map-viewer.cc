#include "graphics.h"
#include "led-matrix.h"
#include "city.h"

#include <Eigen/Dense>

#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

vector<string> tokenize_csv_line(string line);
static void print_usage(const char *prog_name);
static void transform_coords(vector<City> *all_cities, vector<City> *ref_cities);
static vector<City> get_all_cities();
static vector<City> get_ref_cities(vector<City> *all_cities, string ref_string);

int main(int argc, char *argv[])  {
    const Color BG_COLOR = COLOR_GREEN;
    const Color FG_COLOR = COLOR_RED;

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

    cout << "Getting all cities. This might take a minute." << endl;
    vector<City> all_cities = get_all_cities();

    cout << "Getting reference cities." << endl;
    vector<City> ref_cities = get_ref_cities(&all_cities, ref_cities_string);

    cout << "Transforming coordinates." << endl;
    transform_coords(&all_cities, &ref_cities);

    // Display all cities in background color.
    for(const auto& city: all_cities)
        matrix->SetPixel(city.x, city.y, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b);

    // Display reference cities in foreground color.
    for(const auto& city: ref_cities)
        matrix->SetPixel(city.x, city.y, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b);

    int sleep_s = 10;
    cout << "Viewing map for " << sleep_s << " seconds." << endl;
    sleep(sleep_s);

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