#pragma once

#include "ast/query/queries/create.h"
#include "ast/query/query_provider.h"
#include "ds/strings.h"   // string
#include "errors/error.h" // err_t
#include "paging/pager.h"
#include "virtual_machine.h"

typedef struct
{
  pager *pager;
  query_provider *qspce;
  vm *vm;
} database;

database *db_open (const string fname, error *e);
void db_close (database *db);
err_t db_execute (database *db, query *q, error *e);
