#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

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



void parse_first_flight(char *json_response) {
    cJSON *root = cJSON_Parse(json_response);
    cJSON *states = cJSON_GetObjectItem(root, "states");


    cJSON *first_item = cJSON_GetArrayItem(states, 0);

    char *test = cJSON_Print(cJSON_GetArrayItem(first_item, 0));

    printf("%s\n", test);


    



}



int main() {
    printf("Testing cJSON...\n");
    
    char *seattle_data = fetch_opensky();

    parse_first_flight(seattle_data);
    
    
    return 0;
}