#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "flight.h"

int init_database(sqlite3 **db);
int insert_flight(sqlite3 *db, Flight *flight);
void close_database(sqlite3 *db);
int begin_transaction(sqlite3 *db);
int commit_transaction(sqlite3 *db);
int rollback_transaction(sqlite3 *db);


#endif

