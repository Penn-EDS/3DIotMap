#include "city.h"
#include "graphics.h"
#include "led-matrix.h"

#include <Eigen/Dense>

#include <fstream>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

vector<string> tokenize_csv_line(string line);
static void interrupt_handler(int signal);
static void print_usage(const char *prog_name);
static void transform_coords(vector<City> *all_cities, 
    vector<City> *ref_cities);
static void set_pixel_remmaped(RGBMatrix *matrix, int x, int y, uint8_t r, 
    uint8_t g, uint8_t b);
static vector<City> load_all_cities();
static vector<City> load_ref_cities(vector<City> *all_cities, string ref_string);

volatile bool interrupt_received = false;

int main(int argc, char *argv[])  {

    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv);
    if (matrix == NULL)
        return 1;

    string ref_cities_string = 
        "Olympia,WA,19,4,Augusta,ME,109,10,Austin,TX,60,51";
    bool show_ref_cities = false;
    bool use_remapper = false;

    // Parse command-line options.
    while (true) {
        static struct option long_options[] = {
            {"ref-string", required_argument, 0, 'r'},
            {"show-ref", no_argument, 0, 's'},
            {"use-remapper", no_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int opt = getopt_long(argc, argv, "r:sm", long_options, &option_index);
        if (opt == -1)
            break;
        switch (opt) {
            case 'r':
                ref_cities_string = optarg;
                break;
            case 's':
                show_ref_cities = true;
                break;
            case 'm':
                use_remapper = true;
                break;
            case '?':
                print_usage(argv[0]);
                // Fall through.
            default:
                return 1;
            }
    }

    vector<City> all_cities = load_all_cities();
    vector<City> ref_cities = load_ref_cities(&all_cities, ref_cities_string);
    transform_coords(&all_cities, &ref_cities);

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

    for(const auto& city: all_cities) {

        const Color COLOR_MIN = COLOR_YELLOW;
        const Color COLOR_MAX = COLOR_RED;

        long int positive = statePositive[city.state];
        
        // Map from logarithmic (positive_min..positive_max) range to
        // (0..1) linear range.
        float log_min = positive_min + 1; //  Avoid division by zero.
        float log_max = positive_max + 1; //  Avoid division by zero.
        float percent = (log(positive) - log(log_min))/(log(log_max) - log(log_min));

        // Map from (0..1) range to (COLOR_MIN.r..COLOR_MAX.r) range.
        int r = (COLOR_MIN.r < COLOR_MAX.r) ? 
            COLOR_MIN.r + abs(COLOR_MAX.r - COLOR_MIN.r) * percent : 
            COLOR_MIN.r - abs(COLOR_MAX.r - COLOR_MIN.r) * percent;

        // Map from (0..1) range to (COLOR_MIN.g..COLOR_MAX.g) range.
        int g = (COLOR_MIN.g < COLOR_MAX.g) ? 
            COLOR_MIN.g + abs(COLOR_MAX.g - COLOR_MIN.g) * percent : 
            COLOR_MIN.g - abs(COLOR_MAX.g - COLOR_MIN.g) * percent;

        // Map from (0..1) range to (COLOR_MIN.b..COLOR_MAX.b) range.
        int b = (COLOR_MIN.b < COLOR_MAX.b) ? 
            COLOR_MIN.b + abs(COLOR_MAX.b - COLOR_MIN.b) * percent : 
            COLOR_MIN.b - abs(COLOR_MAX.b - COLOR_MIN.b) * percent;

        if (use_remapper)
            set_pixel_remmaped(matrix, city.x, city.y, r, g, b);
        else
            matrix->SetPixel(city.x, city.y, r, g, b);
    }

    // Display reference cities in a different color.
    if (show_ref_cities) {
        for(const auto& city: ref_cities) {
            if (use_remapper)
                set_pixel_remmaped(matrix, city.x, city.y, COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b);
            else
                matrix->SetPixel(city.x, city.y, COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b);
        }
    }

    signal(SIGINT, interrupt_handler);
    cout << "Done. Press Ctrl+C to exit." << endl;
    do {
        // TODO Update here.
        sleep(1); // Avoid flickering.
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
    cerr <<
    "Usage: sudo ./map-viewer [options]\n\nOptions:\n\t--ref-string, -r : Comma"
    "-separated reference string with format, \"<city1>,<st1>,<x1>,<y1>,<city2>"
    ",<st2>,<x2>,<y2>,<city3>,<st3>,<x3>,<y3>\" (default=\"Olympia,WA,19,4,Augu"
    "sta,ME,109,10,Austin,TX,60,51\")\n\t--led-cols       : Number of columns i"
    "n one panel (default=32).\n\t--led-rows       : Number of rows in one pane"
    "l (default=32).\n\t--led-chain      : Number of daisy-chained panels (defa"
    "ult=1).\n\t--led-parallel   : Number of parallel chains (range=1..3, defau"
    "lt=1).\n\nFlags:\n\t--show-ref, -s     : Show reference cities in white.\n"
    "\t--use-remapper, -m : Use the remapper for the setup at Penn."
    << endl;
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

        // Transform spherical coordinates into planar coordinates. The plate 
        // carrée projection simply maps x to be the value of the longitude 
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

vector<City> load_all_cities() {
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
        cities.push_back(new_city);
    }
    return cities;
}

vector<City> load_ref_cities(vector<City> *all_cities, string ref_string) {
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
            ref_cities.push_back(new_city);
        }
    }

    return ref_cities;
}

static void interrupt_handler(int signal) {
    interrupt_received = true;
}

// This function is temporary and should only be used for the Penn set up. This 
// is not how pixels should be remapped; implement the PixelMapper interface 
// instead. Remapping tests:
//  (132, 34) -> (93, 36)
//  (34, 36) -> (164, 61)
//  (166, 105) -> (22, 70)
//  (7, 1) -> (129, 88)
//  (125, 83) -> (44, 29)
static void set_pixel_remmaped(RGBMatrix *matrix, int x, int y, uint8_t r, 
    uint8_t g, uint8_t b) {

    // Skip if pixel if pixel is not visible.
    if (x < 0 || x >= 192 || y < 0 || y >= 128)
        return;

    int new_x, new_y;
    if (x < 96) {
        new_x = y +  128;
        new_y = 95 - x;
    } else {
        new_x = 127 - y;
        new_y = x - 96;
    }

    matrix->SetPixel(new_x, new_y, r, g, b);
}