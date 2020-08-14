#include "cities-db.h"
#include "graphics.h"
#include "led-matrix.h"

#include <Eigen/Dense>

#include <iostream>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

static void print_usage(const char *prog_name);

int main(int argc, char *argv[])  {

    const Color BG_COLOR = COLOR_BLUE;
    const Color FG_COLOR = COLOR_RED;

    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv);
    if (matrix == NULL)
        return 1;

    // Parse remaning arguments.
    string ref_cities;
    int opt;
    while ((opt = getopt(argc, argv, "r:")) != -1) {
        switch (opt) {
            case 'r':
                ref_cities = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    CitiesDB cities_db("uscities.csv");

    string city1, state1, city2, state2, city3, state3;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
    float lng1, lat1, lng2, lat2, lng3, lat3;
    
    // Parse reference cities string.
    try {
        vector<string> ref_tokens = cities_db.tokenize_csv_line(ref_cities);
        city1 = ref_tokens[0], state1 = ref_tokens[1], x1 = stoi(ref_tokens[2]),  y1 = stoi(ref_tokens[3]);
        city2 = ref_tokens[4], state2 = ref_tokens[5], x2 = stoi(ref_tokens[6]),  y2 = stoi(ref_tokens[7]);
        city3 = ref_tokens[8], state3 = ref_tokens[9], x3 = stoi(ref_tokens[10]), y3 = stoi(ref_tokens[11]);
    } catch (...) {
        cerr << "Error parsing reference cities string." << endl;
    }

    string lng1str = cities_db.get_field(city1, state1, CitiesDB::Index::LNG);
    string lat1str = cities_db.get_field(city1, state1, CitiesDB::Index::LAT);
    string lng2str = cities_db.get_field(city2, state2, CitiesDB::Index::LNG);
    string lat2str = cities_db.get_field(city2, state2, CitiesDB::Index::LAT);
    string lng3str = cities_db.get_field(city3, state3, CitiesDB::Index::LNG);
    string lat3str = cities_db.get_field(city3, state3, CitiesDB::Index::LAT);

    lng1 = atof(lng1str.c_str());
    lat1 = atof(lat1str.c_str());
    lng2 = atof(lng2str.c_str());
    lat2 = atof(lat2str.c_str());
    lng3 = atof(lng3str.c_str());
    lat3 = atof(lat3str.c_str());

    // Find the transformation required to get from (<lng>, <lng>) to (<x>, <y>).
    Eigen::MatrixXf A(6, 6);
    A << 
        lng1, lat1, 1, 0, 0, 0,
        0, 0, 0, lng1, lat1, 1,
        lng2, lat2, 1, 0, 0, 0,
        0, 0, 0, lng2, lat2, 1,
        lng3, lat3, 1, 0, 0, 0,
        0, 0, 0, lng3, lat3, 1;
    Eigen::MatrixXf A_prime(6, 1);
    A_prime << x1, y1, x2, y2, x3, y3;
    Eigen::MatrixXf trans = A.inverse() * A_prime;

    vector<string> tokens;
    cities_db.reset();
    while (cities_db.next_record(&tokens)) {

        string lngStr = tokens[CitiesDB::Index::LNG];
        string latStr = tokens[CitiesDB::Index::LAT];

        float lng = atof(lngStr.c_str());
        float lat = atof(latStr.c_str());

        // Transform spherical coordinates into planar coordinates.
        // The plate carrée projection simply maps x to be the value of the longitude 
        // and y to be the value of the latitude.
        float planar_x = lng;
        float planar_y = lat;
        
        // Transform planar coordinates into matrix coordinates.
        Eigen::MatrixXf B(2, 6);
        B <<
            planar_x, planar_y, 1, 0, 0, 0,
            0, 0, 0, planar_x, planar_y, 1;
        Eigen::MatrixXf B_prime = B * trans;
        int matrix_x = int(B_prime(0, 0));
        int matrix_y = int(B_prime(1, 0));

        // Display city in background color.
        matrix->SetPixel(matrix_x, matrix_y, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b);    
    }

    // Display reference cities in foreground color.
    matrix->SetPixel(x1, y1, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b);
    matrix->SetPixel(x2, y2, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b);
    matrix->SetPixel(x3, y3, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b);

    int sleep_s = 10;
    cout << "Viewing map for " << sleep_s << " seconds..." << endl;
    sleep(sleep_s);

    matrix->Clear();
}

static void print_usage(const char *prog_name) {
    cerr << "Usage: sudo " << prog_name << " [options]" << endl;
    cerr << "\tDisplay US cities using matrix coordinates of three cities as reference." << endl;
    cerr << "Options:" << endl;
    cerr << "\t-r <string> : Comma-separated reference string with format," << endl;
    cerr << "\t\t      \"<city1>,<state1>,<x1>,<y1>,<city2>,<state2>,<x2>,<y2>,<city3>,<state3>,<x3>,<y3>\"" << endl;
    cerr << "\t\t      (default=\"Seattle,WA,10,10,El Paso,TX,60,50,Portland,ME,110,10\")" << endl;
    // TODO Print matrix options.
}