#pragma once

#include "ast/query/query_provider.h"
#include "ds/strings.h"   // string
#include "errors/error.h" // err_t
#include "paging/pager.h"
#include "virtual_machine.h"

/**
 * A database holds the primary resources
 * used by the database including
 * 1. The pager   - holds pages in memory and reads from disk
 * 2. qspce       - memory management for queries
 */
typedef struct
{
  pager *pager;
  query_provider *qspce;
} database;

database *db_open (const string fname, error *e);
err_t db_close (database *d, error *e);
