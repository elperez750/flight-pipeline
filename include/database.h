#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "flight.h"

int init_database(sqlite3 **db);
int insert_flight(sqlite3 *db, Flight *flight);
void close_database(sqlite3 *db);

#endif

