#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "include/flight.h"



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


void parse_flight(cJSON *state_array, Flight *f, int timestamp) {
    f->time = timestamp;

    cJSON *icao24 = cJSON_GetArrayItem(state_array, 0);
    cJSON *callsign = cJSON_GetArrayItem(state_array, 1);
    cJSON *origin_country = cJSON_GetArrayItem(state_array, 2);
    cJSON *longitude = cJSON_GetArrayItem(state_array, 5);
    cJSON *latitude = cJSON_GetArrayItem(state_array, 6);
    cJSON *velocity = cJSON_GetArrayItem(state_array, 9);
    cJSON *last_contact = cJSON_GetArrayItem(state_array, 4);
    cJSON *geo_altitude = cJSON_GetArrayItem(state_array, 13);


    strncpy(f->icao24, icao24->valuestring, sizeof(f->icao24) - 1);
    strncpy(f->callsign, callsign->valuestring, sizeof(f->callsign) - 1);
    strncpy(f->origin_country, origin_country->valuestring, sizeof(f->origin_country) - 1);
    f->longitude = longitude->valuedouble;
    f->latitude = latitude->valuedouble;
    f->velocity = velocity->valuedouble;
    f->last_contact = last_contact->valueint;
    f->geo_altitude = geo_altitude->valuedouble;

}



void parse_first_flight(char *json_response) {
    Flight flight;

    cJSON *root = cJSON_Parse(json_response);
    cJSON *time = cJSON_GetObjectItem(root, "time");
    cJSON *states = cJSON_GetObjectItem(root, "states");


    cJSON *first_item = cJSON_GetArrayItem(states, 0);

   
    char *test = cJSON_Print(cJSON_GetArrayItem(first_item, 0));

    parse_flight(first_item, &flight, time->valueint);

    
    for (int i = 0; i < cJSON_GetArraySize(first_item); i++) {
        cJSON *item = cJSON_GetArrayItem(first_item, i);
        
    }
    


    printf("%s\n", test);
    printf("%d\n", time->valueint);


    



}



int main() {
    printf("Testing cJSON...\n");
    
    char *seattle_data = fetch_opensky();

    parse_first_flight(seattle_data);
    
    
    return 0;
}