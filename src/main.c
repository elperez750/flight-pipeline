#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "flight.h"



#include "../lib/cJSON.h"

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;



// Callback function - libcurl calls this when data arrives
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    // real size is the current size times bytes
    size_t realsize = size * nmemb;

    // We create a buffer from our previous buffer which is userp
    ResponseBuffer *buffer = (ResponseBuffer *)userp;
    

    // We then move the data to a space that is the current buffer size + recalculated size
    char *ptr = realloc(buffer->data, buffer->size + realsize + 1);

    // If no ptr that means we ran out of memory
    if (!ptr) {
        printf("Out of memory!\n");
        return 0;
    }


    // The buffer data will now start where the new ptr starts
    buffer->data = ptr;

    // we copy the contents into the new allocated memory
    memcpy(&(buffer->data[buffer->size]), contents, realsize);

    // Increase the current buffer size by real size
    buffer->size += realsize;

    // null terminate
    buffer->data[buffer->size] = 0; 
    
    return realsize;
}



char* fetch_opensky() {
    CURL *curl;
    CURLcode res;
    ResponseBuffer buffer = {0};
    
    buffer.data = malloc(1);
    buffer.size = 0;
    

    // Set up curl request
    curl = curl_easy_init();

    // If we didn't get that then we just say failed to initialize it 
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return NULL;
    }
    
    // Seattle bounding box. This is the url
    curl_easy_setopt(curl, CURLOPT_URL, 
        "https://opensky-network.org/api/states/all?lamin=47.0&lomin=-123.0&lamax=48.0&lomax=-121.5");

    // Function that will perform the writing logic
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // What will hold the information, which will be the buffer
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
    

    // We then call this
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(buffer.data);
        curl_easy_cleanup(curl);
        return NULL;
    }
    

    // We clean this up and return the data
    curl_easy_cleanup(curl);
    return buffer.data;
}


/*

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


*/

double safe_get_number(cJSON *array, int index) {
    // We get the number by passing in the array and the index

    // We seacrh the array at the given index
    cJSON *item = cJSON_GetArrayItem(array, index);

    // Check to see if we got something and if that something is a number
    if (item && cJSON_IsNumber(item)) {

        // We return the value of the item, which is a double
        // This is automatically converted to a float and int 

        return item->valuedouble;
    }

    return 0.0;
}

void safe_get_string(cJSON *array, int index, char *dest, size_t max_len) {
    cJSON *item = cJSON_GetArrayItem(array, index);

    if (item && cJSON_IsString(item)) {
        strncpy(dest, item->valuestring, max_len - 1);
        dest[max_len - 1] = '\0';
    }
    else {
        strcpy(dest, "N/A");
    }
}


void parser_helper(cJSON *state_array, Flight *f, int timestamp) {

    // Pass in the time to the struct
    f->time = timestamp;

    // We need to check if values are null before hand

    // Storing the strings inside of the struct
    safe_get_string(state_array, 0, f->icao24, sizeof(f->icao24));
    safe_get_string(state_array, 1, f->callsign, sizeof(f->callsign));
    safe_get_string(state_array, 2, f->origin_country, sizeof(f->origin_country));

    // Storing numbers inside of the struct
    f->last_contact = (int)safe_get_number(state_array, 4);
    f->longitude = (float)safe_get_number(state_array, 5);
    f->latitude = (float)safe_get_number(state_array, 6);
    f->velocity = (float)safe_get_number(state_array, 9);
    f->geo_altitude = (float)safe_get_number(state_array, 13);



}



void parse_first_flight(char *json_response) {
    Flight flight;

    cJSON *root = cJSON_Parse(json_response);
    cJSON *time = cJSON_GetObjectItem(root, "time");
    cJSON *states = cJSON_GetObjectItem(root, "states");


    cJSON *first_item = cJSON_GetArrayItem(states, 3);

   


    // First item includes all fields
    // flight will be the struct where we include the data
    // we are passing in time to pass into the struct.
    parser_helper(first_item, &flight, time->valueint);

    
    printf("First flight data:\n");
    printf("Time: %d\n", flight.time);
    printf("ICAO24: %s\n", flight.icao24);
    printf("Callsign: %s\n", flight.callsign);
    printf("Origin Country: %s\n", flight.origin_country);
    printf("Latitude: %f\n", flight.latitude);
    printf("Longitude: %f\n", flight.longitude);
    printf("Geo Altitude: %f\n", flight.geo_altitude);
    printf("Velocity: %f\n", flight.velocity);
    printf("Last Contact: %d\n", flight.last_contact);  


    cJSON_Delete(root);



}



int main() {
    printf("Testing cJSON...\n");
    
    char *seattle_data = fetch_opensky();

    parse_first_flight(seattle_data);
    
    
    return 0;
}