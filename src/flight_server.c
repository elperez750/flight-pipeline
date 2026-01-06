
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h> // Include for EXIT_FAILURE



// This is the struct that will hold the path and the query

// The path for example would be "curl "https://opensky-network.org/api/states/all"
// The query would be "?name=elliott&age=22"

typedef struct
{
    char path[256];  // Everything before the question mark
    char query[256]; // Everything after the question mark
} ParsedRoute;

// This is the individual query param for a route
// From the last example, the key would be name and age
// The value would be elliott and 22
typedef struct
{
    char key[256];
    char value[256];
} QueryParam;


// This holds multiple query params
typedef struct
{
    QueryParam params[10];
    int count;
} QueryParams;


int city_count = 0;

// JSON response builder
void send_json_response(int client_fd, char *json_data)
{
    char response[4096];
    int len = snprintf(response, sizeof(response),
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: %zu\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type\r\n"
                       "\r\n"
                       "%s",
                       strlen(json_data),
                       json_data);
    write(client_fd, response, len);
}

void split_query_params(QueryParams *qp, char *query)
{
    // "/api/game?rounds=5&difficulty=hard"

    // "rounds=5&difficulty=hard&location=seattle"
    // If there is no query we return

    qp->count = 0;
    if (query == NULL)
        return;

    char query_copy[256];

    // query would be "rounds=5&difficulty=hard"
    // We copy this into our query copy string
    strncpy(query_copy, query, sizeof(query_copy) - 1);

    // We then end it using '\0'
    query_copy[sizeof(query_copy) - 1] = '\0';

    // saveptr1 is for each param
    // ex: "rounds=5", "difficulty=hard"
    // saveptr2 is for each query in the param
    // ex: round, 5

    char *saveptr1, *saveptr2;

    // This will split on the &, basically giving us a query each time
    char *pair = strtok_r(query_copy, "&", &saveptr1);

    while (pair != NULL)
    {

        // The key will be rounds
        char *key = strtok_r(pair, "=", &saveptr2);

        // The value will be 5
        char *value = strtok_r(NULL, "=", &saveptr2);

        // If we get values for bot we will simply save them into our structure
        if (key && value)
        {
            size_t key_size = sizeof(qp->params[qp->count].key);
            size_t value_size = sizeof(qp->params[qp->count].value);

            strncpy(qp->params[qp->count].key, key, key_size - 1);
            qp->params[qp->count].key[key_size - 1] = '\0';

            strncpy(qp->params[qp->count].value, value, value_size - 1);
            qp->params[qp->count].value[value_size - 1] = '\0';
            printf("Key: %s, Value: %s\n", qp->params[qp->count].key, qp->params[qp->count].value);
            qp->count++;
        }

        pair = strtok_r(NULL, "&", &saveptr1);
    }

    // route: /api/game
    // query: rounds=5&diffculty=hard
    // rounds = 5
    // difficulty = hard
}


ParsedRoute *split_route(char *route)
{
    ParsedRoute *result = malloc(sizeof(ParsedRoute));

    char *path_end = strstr(route, "?");

    if (path_end == NULL)
    {
        strcpy(result->path, route);
        result->query[0] = '\0';
        return result;
    }

    size_t path_length = path_end - route;

    strncpy(result->path, route, path_length);
    result->path[path_length] = '\0';

    char *query_start = path_end + 1;

    strcpy(result->query, query_start);

    return result;

    // Your code here
    // Hint: Find the '?', calculate sizes, use strncpy

    // /api/game?rounds=5
    // /api/game is the path
    // rounds=5 is the query
}

// Takes in the request and the header name
char *get_header_value(char *request, char *header_name)
{

    // Header size.
    // Example: Host, user_agent
    size_t header_size = strlen(header_name);

    // This finds the end of the first line, denoted by \r\n
    char *line = strstr(request, "\r\n");

    // If we don't get the line, that means that we don't have one
    if (!line)
        return NULL;

    // Otherwise, we add two to the line in order to get to the next line and skip \r\n
    line += 2;

    // While we have a line and the line is not \r which is the end character.
    while (line && *line != '\r')
    {

        // We use strncmp becuase this lets us specify the amount of chars to compare
        // We compare line with header_name up to header_size characters
        // This returns 0 if the strings are equal
        if (strncmp(line, header_name, header_size) == 0)
        {

            // If they are equal, we see where the ": " starts. Thats because we need to skip over it to get the header we want
            char *colon = strstr(line, ": ");
            if (colon)
            {

                // We get where our header starts
                char *line_start = colon + 2;

                // Go to the end of the line
                char *line_end = strstr(line_start, "\r\n");

                // If we run out of space, return NULL
                if (!line_end)
                {
                    return NULL;
                }

                // Otherwise, we want to get the size of the header
                // We subtract where line end address is from line start address
                size_t result_size = line_end - line_start;

                // allocate result size + 1 for the header.
                // We will copy the header into result
                // We do result_size + 1 to include '\0'
                char *result = malloc(result_size + 1);
                if (!result)
                {
                    return NULL;
                }

                // We will copy result_size characters from line_start into result
                strncpy(result, line_start, result_size);
                result[result_size] = '\0';

                return result;
            }
        }

        // Otherwise, we find the end of the string.
        line = strstr(line, "\r\n");

        // If we have a line we add two to jump over \r\n. We enter the while loop again
        if (line)
            line += 2;
    }

    // If we exit without returning anything, we return null
    return NULL;
}


char *find_query_start(char *route)
{
    // Your code here
    // Hint: You've used strstr() and strchr() before...

    // /api/game?rounds=5

    char *query_start = strstr(route, "?");
    if (query_start == NULL)
    {
        return NULL;
    }

    else
    {
        return query_start + 1;
    }

    // check if string has ?
    // if not, we return null
    //
}

void test_query(char *input)
{
    char *result = find_query_start(input);

    if (result != NULL)
    {
        printf("Input: %-20s | Result: %s\n", input, result);
    }
    else
    {
        printf("Input: %-20s | Result: NULL\n", input);
    }
}

int main()
{
    // Create socket
    //  AF_INET is IPv4
    // SOCK_STREAM is TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Address structure
    /*
    Its like having the following
    typedef struct {
        short sin_family;
        struct in_addr sin_addr
        unsigned short sin_port
    }

    */

    int opt = 1;
    // This allows the socket to bind even if the port is in TIME_WAIT state.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;         // Address family (IPv4 vs IPv6)
    address.sin_addr.s_addr = INADDR_ANY; // IP address
    address.sin_port = htons(8080);       // Port 8080

    /*
    Why htons?
    computer might store 8080 as [31, 144] in memory
    network protocol requires [144, 31] big endian
    */

    // Binding server to address
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // Listen for connections. If four people try to connect simotaneously, the fourth gets rejected
    listen(server_fd, 3);

    
    while (1)
    {

        // File descriptor
        int client_fd = accept(server_fd, NULL, NULL);

        char buffer[1024];

        // Pulls data from client_fd into buffer. This will read up to 1023 bytes (saves room for null terminator)
        int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        buffer[bytes_read] = '\0';

        printf("This is the buffer %s\n", buffer);
        char method[16];
        char route[256];
        char protocol[16];

        int items_assigned = sscanf(buffer, "%15s %255s %15s", method, route, protocol);
     
        // If request is succesful
        char *success_response = "HTTP/1.1 200 OK\r\n\r\nWelcome";

        // If our request fails
        char *error_response = "HTTP/1.1 404 Not Found\r\n\r\nFuck you";
        // From the route we extract the route and the query parameters

        ParsedRoute *pr = split_route(route);

        // This route is to start the game
        

    
        char *test_query_one = "/api/game?rounds=5";
        char *test_query_two = "/about";
        char *test_query_three = "/?test=1";

        test_query(test_query_one);
        test_query(test_query_two);
        test_query(test_query_three);

        ParsedRoute *route_to_use = split_route(test_query_one);

        printf("This is the route: %s\n", route_to_use->path);
        printf("This is the query: %s\n", route_to_use->query);

        QueryParams qp;

        split_query_params(&qp, "rounds=5&difficulty=hard&map=world");

        free(route_to_use);

        close(client_fd);
    }

    close(server_fd);
}
