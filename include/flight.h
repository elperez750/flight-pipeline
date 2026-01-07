#ifndef FLIGHT_H
#define FLIGHT_H
#include <time.h>

/*

icao24
callsign
origin_country
longitude
latitude
altitude (pick one: baro or geo)
velocity
last_contact (so you know when data was fresh)

*/
typedef struct {
    int time; // will be passed into function
    char icao24[16]; // index 0
    char callsign[8]; // index 1
    char origin_country[20]; // index 2
    float latitude; // index 6
    float longitude; // index 5
    float geo_altitude; // index 13
    float velocity; // index 9
    int last_contact; // index 4


} Flight;



typedef struct {
    Flight *flights;
    size_t flight_count;

} FlightData;



#endif