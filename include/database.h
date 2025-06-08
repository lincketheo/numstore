#pragma once

#include "ds/strings.h"   // string
#include "errors/error.h" // err_t

typedef struct database_s database;

database *db_open (const string fname, error *e);

void db_close (database *db);
