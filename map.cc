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

    // Minimum and maximum longitudes and latitudes to display.
    const float MIN_LNG = -124.8228;
    const float MAX_LNG = -69.6276;
    const float MIN_LAT = 26.2482; 
    const float MAX_LAT = 49.2335; 

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

        // Map lng and lat from old ranges (MIN_LNG..MAX_LNG and MIN_LAT..MAX_LAT) 
        // to new ranges (0..matrix->width() and 0..matrix->height()).
        float x = matrix->width() * (lng - MIN_LNG) / (MAX_LNG - MIN_LNG);
        float y = matrix->height() * (lat - MIN_LAT) / (MAX_LAT - MIN_LAT);
        
        // Flip vertical.
        y = (matrix->height() - y);

        matrix->SetPixel(x, y, 255, 0, 0);    
    }
    sleep(5);
    matrix->Clear();
}