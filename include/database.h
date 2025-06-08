#pragma once

#include "ast/query/queries/create.h"
#include "ds/strings.h"   // string
#include "errors/error.h" // err_t

typedef struct database_s database;

database *db_open (const string fname, error *e);

void db_close (database *db);

err_t db_execute (database *db, query *q, error *e);
