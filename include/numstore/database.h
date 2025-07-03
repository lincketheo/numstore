#pragma once

#include "core/ds/strings.h"   // string
#include "core/errors/error.h" // err_t

#include "numstore/paging/pager.h" // TODO

typedef struct
{
  pager *pager;
} database;

database *db_open (const string fname, error *e);
err_t db_close (database *d, error *e);
