#include "cities-db.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

CitiesDB::CitiesDB(string db_filename) {
    this->db_filename = db_filename;
    this->db_stream.open(db_filename);
    if (!this->db_stream.is_open()) {
        cerr << "Error opening: '" << db_filename << "'" << endl;
        throw 0;
    }
}

vector<string> CitiesDB::tokenize_csv_line(string line) {
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

void CitiesDB::reset() {
    this->db_stream.seekg(0);

    // Skip first line (row) with column names.
    string line;
    getline(this->db_stream, line);

}

bool CitiesDB::next_record(vector<string> *tokens) {
    string line;
    if (getline(this->db_stream, line)) {
        *tokens = tokenize_csv_line(line);
        return true;
    } else {
        return false;
    }
}

string CitiesDB::get_field(string city, string state_id, CitiesDB::Index field_index) {

    string line;
    
    CitiesDB::reset();

    // Search for matching city and state ID.
    while (getline(this->db_stream, line)) {
        vector<string> tokens = CitiesDB::tokenize_csv_line(line);

        // Skip if no match.
        if (tokens[CitiesDB::Index::CITY].compare(city) || tokens[CitiesDB::Index::STATE_ID].compare(state_id))
            continue;

        string field = tokens[field_index];
        return field;
    }

    // Field not found.
    return "";
    
}

