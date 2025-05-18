#pragma once

#include "ast/query/qspace_provider.h"
#include "paging/pager.h"

typedef struct
{
  pager *pager;
  qspce_prvdr *qspce;
} database;

err_t db_create (const string fname, error *e);

err_t db_open (database *dest, const string fname, error *e);

void db_close (database *d);
