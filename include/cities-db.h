#ifndef CITIES_DB_H
#define CITIES_DB_H

#include <string>
#include <fstream>
#include <vector>

using namespace std;

class CitiesDB {

    private:
        string db_filename;
        ifstream db_stream;
        

    public:

        enum Index { 
            CITY, CITY_ASCII, STATE_ID, STATE_NAME, COUNTY_FIPS, COUNTY_NAME,
            COUNTY_FIPS_ALL, COUNTY_NAME_ALL, LAT, LNG, POPULATION, DENSITY,
            SOURCE, MILITARY, INCORPORATED, TIMEZONE, RANKING, ZIPS, ID
        };

        CitiesDB(string db_filename);
        vector<string> tokenize_csv_line(string line);
        string get_field(string city, string state, CitiesDB::Index field);
        void reset();
        bool next_record(vector<string> *tokens);


        

        

};

#endif