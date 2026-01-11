#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "flight.h"
#include "database.h"
#include "../lib/cJSON.h"
#include <unistd.h>




typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

void save_response_to_file(const char *data, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f) {
        fputs(data, f);
        fclose(f);
        printf("Saved API response to %s\n", filename);
    }
}

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


char* load_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(fsize + 1);
    fread(data, 1, fsize, f);
    data[fsize] = '\0';
    
    fclose(f);
    return data;
}


// This function will make an http request to open sky and return a json string
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
        "https://opensky-network.org/api/states/all");

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




int main() {
    printf("Testing cJSON...\n");
    sqlite3 *db;
    init_database(&db);
    
    while (1) {

        //char *world_flight_data = fetch_opensky();
        //save_response_to_file(world_flight_data, "test_data.json");


        char *world_flight_data = load_from_file("test_data.json");


        // This gets the cJSON root
        cJSON *root = cJSON_Parse(world_flight_data);

        // This will get us the time
        cJSON *flight_time = cJSON_GetObjectItem(root, "time");

        // This gets us the states array, which holds all flight data
        cJSON *states = cJSON_GetObjectItem(root, "states");

        // Get number of flights in the Seattle area
        int num_flights = cJSON_GetArraySize(states);
        printf("Number of flights in the world currently: %d\n", num_flights);


        Flight *fleet = malloc(num_flights * sizeof(Flight));

        // We pass in the flight to process, We pass in the allocated memory
        


        time_t start = time(NULL);
        begin_transaction(db);

        int success = 1;
        for (int i = 0; i < num_flights; i++) {
            parser_helper(cJSON_GetArrayItem(states, i), &fleet[i], flight_time->valueint);

            // If the flight does not return the code expected, then one flight was not processed correctly
            if(insert_flight(db, &fleet[i]) != SQLITE_DONE) {
                fprintf(stderr, "Insert failed for flight %d\n", i);
                success = 0;
                break;
            }
        }



         // if success remains 1, then we succesfully inserted all flights, so we commit them all to disk at once
         if (success) {
           commit_transaction(db);
            time_t end = time(NULL);


            double time_spent = difftime(end, start);
             printf("Inserted %d flights in %.0f seconds\n", num_flights, time_spent);
         }   



        // We rollback our inserts. If one insert fail then all inserts fail.
         else {
             rollback_transaction(db);

         }
        



        free(world_flight_data);
        free(fleet);
        cJSON_Delete(root);

        printf("Batch complete. Sleeping for 60 seconds ...\n");
        sleep(60);

    }
    
    
    close_database(db);
    return 0;
}