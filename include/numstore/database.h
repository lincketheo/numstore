#pragma once

#include "compiler/ast/query/query_provider.h"
#include "core/ds/strings.h"   // string
#include "core/errors/error.h" // err_t
#include "numstore/paging/pager.h"
#include "numstore/virtual_machine.h"

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
