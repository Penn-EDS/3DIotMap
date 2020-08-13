#include "led-matrix.h"

#include <Eigen/Dense>
#include <graphics.h>
#include <cmath>
#include <ctgmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

vector<string> tokenize_csv_line(string line);

int main(int argc, char *argv[])  {
    const int LNG_COL_INDEX = 9;
    const int LAT_COL_INDEX = 8;

    const Color BG_COLOR = COLOR_BLUE;
    const Color FG_COLOR = COLOR_RED;

    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv);
    if (matrix == NULL)
        return 1;

    // Open SimpleMaps US cities database CSV.
    string cities_db_filename = "uscities.csv";
    ifstream cities_db(cities_db_filename);
    if (!cities_db.is_open()) {
        cout << "Error opening: '" << cities_db_filename << "'" << endl;
        return 1;
    }

    // Reference city 1: Seattle, WA
    float lng1 = -122.3244, lat1 = 47.6211;
    int x1 = 10, y1 = 10;

    // Reference city 2: El Paso, TX
    float lng2 = -106.4309, lat2 = 31.8479;
    int x2 = 60, y2 = 50;

    // Reference city 3: Portland, ME
    float lng3 = -70.2715, lat3 = 43.6773;
    int x3 = 110, y3 = 10;

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

    string line;

    // Skip first line (row) with column names.
    getline(cities_db, line);

    while (getline(cities_db, line)) {

        vector<string> tokens = tokenize_csv_line(line);

        string lngStr = tokens[LNG_COL_INDEX];
        string latStr = tokens[LAT_COL_INDEX];

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

    int sleep_s = 5;
    cout << "Viewing map for " << sleep_s << " seconds..." << endl;
    sleep(sleep_s);

    matrix->Clear();
}

vector<string> tokenize_csv_line(string line) {
    vector<string> tokens;
    stringstream line_stream(line);
    string token;
    while (getline(line_stream, token, ',')) {
        
        // Remove surrounding quotation marks.
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        
        tokens.push_back(token);
    }
    return tokens;
}