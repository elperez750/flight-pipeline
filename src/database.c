#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include <sqlite3.h>
#include <time.h>



// open the database, create the table, return success/failure
int init_database(sqlite3 **db) {
    int rc = sqlite3_open("flights.db", db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    printf("Database opened successfully.\n");

    char *sql = "CREATE TABLE IF NOT EXISTS FLIGHTS("
        "id INTEGER PRIMARY KEY, "
        "time INTEGER, "
        "icao24 TEXT, "
        "callsign TEXT, "
        "origin_country TEXT, "
        "latitude REAL, "
        "longitude REAL, "
        "geo_altitude REAL, "
        "velocity REAL, "
        "last_contact INTEGER"
    ")";


    rc = sqlite3_exec(*db, sql, 0, 0 , 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create table: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    return SQLITE_OK;
}



int insert_flight(sqlite3 *db, Flight *flight) {
    sqlite3_stmt *stmt;


    char *sql = "INSERT INTO FLIGHTS (time, icao24, callsign, origin_country, latitude, longitude, geo_altitude, velocity, last_contact) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";


    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    sqlite3_bind_int(stmt, 1, flight->time);
    sqlite3_bind_text(stmt, 2, flight->icao24, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, flight->callsign, -1, SQLITE_TRANSIENT);    
    sqlite3_bind_text(stmt, 4, flight->origin_country, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, flight->latitude);
    sqlite3_bind_double(stmt, 6, flight->longitude);
    sqlite3_bind_double(stmt, 7, flight->geo_altitude);
    sqlite3_bind_double(stmt, 8, flight->velocity);
    sqlite3_bind_int(stmt, 9, flight->last_contact);


    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE) {
        fprintf(stderr, "Values not properly inserted into database: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return rc;
}


void close_database(sqlite3 *db) {
    sqlite3_close(db);
}