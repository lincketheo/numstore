#pragma once

#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "mm/lalloc.h"

typedef enum
{
  QT_CREATE,
  QT_DELETE,
} query_t;

///// QUERY
struct query_s
{
  query_t type;

  union
  {
    create_query *create;
    delete_query *delete;
  };

  /**
   * Every query must have a local allocator for
   * "query" related data. E.g. when I run
   * create foo u32, where is "foo" and "u32"
   * allocated.
   */
  lalloc *qalloc;

  // One error per query
  bool ok;
  error e;
};

void i_log_query (query q);
struct query_s query_error_create (error e);
