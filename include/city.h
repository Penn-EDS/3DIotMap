#ifndef CITY_H
#define CITY_H

using namespace std;

class City {
    public:
        string name;
        string state;
        float lng;
        float lat;
        int x;
        int y;
        City(string name, string state, float lng, float lat, int x = 0, int y = 0);
};

#endif