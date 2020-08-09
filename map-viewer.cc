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

// Returns substring from delimited string.
string substringAtIndex(string line, int index, char delimiter) {
    string substring;
    stringstream line_stream(line);
    for (int i = 0; line_stream.good() && i <= index; i++)
        getline(line_stream, substring, delimiter);
    return substring;
}

int main(int argc, char *argv[])  {
    RGBMatrix::Options matrix_options;
    ifstream cities_db_file;
  
    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv, &matrix_options);
    if (matrix == NULL)
        return 1;

    // Open SimpleMaps US cities database CSV.
    string cities_db_filename = "uscities.csv";
    cities_db_file.open(cities_db_filename);
    if (!cities_db_file.is_open()) {
        fprintf(stderr, "Error opening cities database CSV.\n");
        return 1;
    }

    // Use hardcoded reference cities for now.
 
    // Seattle, WA
    float lng1 = -122.3244, lat1 = 47.6211;
    int x1 = 10, y1 = 10;

    // El Paso, TX
    float lng2 = -106.4309, lat2 = 31.8479;
    int x2 = 60, y2 = 40;

    // Portland, ME
    float lng3 = -70.2715, lat3 = 43.6773;
    int x3 = 110, y3 = 10;

    // Find the affine transformation required to get from (<lng>, <lng>) to (<x>, <y>).
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

    const int LNG_COL_INDEX = 9;
    const int LAT_COL_INDEX = 8;

    string row_line;

    // Skip first line (row) with column names.
    getline(cities_db_file, row_line);

    // Read remaining lines.
    while (getline(cities_db_file, row_line)) {

        string lngStr = substringAtIndex(row_line, LNG_COL_INDEX, ',');
        string latStr = substringAtIndex(row_line, LAT_COL_INDEX, ',');

        // Remove surrounding quotation marks.
        lngStr = lngStr.substr(1, lngStr.size() - 2);
        latStr = latStr.substr(1, latStr.size() - 2);

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

        matrix->SetPixel(matrix_x, matrix_y, COLOR_BLUE.r, COLOR_BLUE.g, COLOR_BLUE.b);    
    }
    cout << "Viewing map..." << endl;
    sleep(5);
    matrix->Clear();
}