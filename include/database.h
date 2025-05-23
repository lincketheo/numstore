#pragma once

#include "ast/query/query_provider.h" // query_provider
#include "errors/error.h"             // err_t
#include "paging/pager.h"             // pager

typedef struct
{
  pager *pager;
  query_provider *qspce;
} database;

err_t db_create (const string fname, error *e);

err_t db_open (database *dest, const string fname, error *e);

void db_close (database *d);
