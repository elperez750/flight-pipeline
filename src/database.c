#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include <sqlite3.h>
#include <time.h>



// open the database, create the table, return success/failure
int init_database(sqlite3 **db) {

    // This will open or create a new db called flights.db

    int rc = sqlite3_open("flights.db", db);


    // If the return code is not SQLITE_OK, then we did not succesfully open the database
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    printf("Database opened successfully.\n");


    // This is basically the sql query
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
        "last_contact INTEGER, "
        "UNIQUE(icao24, time)"
    ")";

    // This will place create the table with the following fields
    rc = sqlite3_exec(*db, sql, 0, 0 , 0);

    // If the rc is not SQLITE_OK, then we failed to create the table.
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create table: %s\n", sqlite3_errmsg(*db));
        return rc;
    }
    return SQLITE_OK;
}

int begin_transaction(sqlite3 *db) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to begin transaction: %s\n", err_msg);
        sqlite3_free(err_msg);

        return rc;
    }

    return SQLITE_OK;
}


int commit_transaction(sqlite3 *db) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, "COMMIT;", 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to commit to database %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}


int rollback_transaction(sqlite3 *db) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, "ROLLBACK;", 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to rollback changes %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;

}

// This function will insert a flight struct into our database
int insert_flight(sqlite3 *db, Flight *flight) {

    // This is the form that will hold our fields.
    sqlite3_stmt *stmt;

    // We are inserting all struct values into our table.
    char *sql = "INSERT OR IGNORE INTO FLIGHTS (time, icao24, callsign, origin_country, latitude, longitude, geo_altitude, velocity, last_contact) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);


    // Binds our actual values into the form
    sqlite3_bind_int(stmt, 1, flight->time);
    sqlite3_bind_text(stmt, 2, flight->icao24, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, flight->callsign, -1, SQLITE_TRANSIENT);    
    sqlite3_bind_text(stmt, 4, flight->origin_country, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, flight->latitude);
    sqlite3_bind_double(stmt, 6, flight->longitude);
    sqlite3_bind_double(stmt, 7, flight->geo_altitude);
    sqlite3_bind_double(stmt, 8, flight->velocity);
    sqlite3_bind_int(stmt, 9, flight->last_contact);


    // Places all values into the db.
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE) {
        fprintf(stderr, "Values not properly inserted into database: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;

    }



    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return changes;
}



// Simply closes the database after using.
void close_database(sqlite3 *db) {
    sqlite3_close(db);
}



void cleanup_old_data(sqlite3 *db, int days_to_keep) {
    int cutoff_time = time(NULL) - (days_to_keep * 24 * 60 * 60);

    char *sql = "DELETE FROM FLIGHTS WHERE TIME < ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, cutoff_time);
    sqlite3_step(stmt);

    int deleted = sqlite3_changes(db);

    sqlite3_finalize(stmt);

    if (deleted > 0) {
        printf("Cleaned up %d records older than %d days\n", deleted, days_to_keep);

    }
}