#include "led-matrix.h"

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
  
    RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv, &matrix_options);
    if (matrix == NULL)
        return 1;

    // Open SimpleMaps US cities database CSV.
    string cities_db_filename = "uscities.csv";
    ifstream cities_db_file;
    cities_db_file.open(cities_db_filename);

    const int LNG_COL_INDEX = 9;
    const int LAT_COL_INDEX = 8;

    // Use a hardcoded affine matrix for now.
    // This matrix was found by assuming the following transformations were desired:
    //     <city>, <state>  (    <lng>,   <lng>)  -->  (<x>, <y>)
    //     Seattle, WA      (-122.3244, 47.6211)  -->  ( 10,  10)
    //     El Paso, TX      (-106.4309, 31.8479)  -->  ( 60,  40)
    //     Portland, ME     ( -70.2715, 43.6773)  -->  (110,  10)

    float affine[6][1] = {
        {1.81988}, {-1.33616}, {296.24636}, {-0.15601}, {-2.05916}, {88.97542}
    };

    if (!cities_db_file.is_open()) {
        fprintf(stderr, "Error opening cities database CSV.\n");
        return 1;
    }

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
        float x = lng;
        float y = lat;
        
        // Transform planar coordinates into matrix coordinates using affine matrix.
        // This requires some matrix multiplication.
        float transformed[2][1] = {
            {0},
            {0}
        };
        float to_transform[2][6] = {
            {x, y, 1, 0, 0, 0},
            {0, 0, 0, x, y, 1}
        };
        int to_transform_rows = sizeof(to_transform) / sizeof(to_transform[0]);
        int to_transform_cols = sizeof(to_transform[0]) / sizeof(to_transform[0][0]);
        int affine_cols = sizeof(affine[0]) / sizeof(affine[0][0]);
        for(int i = 0; i < to_transform_rows; ++i)
            for(int j = 0; j < affine_cols; ++j)
                for(int k = 0; k < to_transform_cols; ++k)
                    transformed[i][j] += to_transform[i][k] * affine[k][j];
        x = transformed[0][0];
        y = transformed[0][1];

        matrix->SetPixel(x, y, 255, 0, 255);    
    }
    cout << "Viewing map..." << endl;
    sleep(5);
    matrix->Clear();
}