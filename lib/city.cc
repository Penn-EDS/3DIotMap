#include "city.h"

using namespace std;

City::City(string name, string state, float lng, float lat, int x, int y) {
    this->name = name;
    this->state = state;
    this->lng = lng;
    this->lat = lat;
    this->x = x;
    this->y = y;
}