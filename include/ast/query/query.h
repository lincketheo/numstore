#pragma once

#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "mm/lalloc.h"

typedef enum
{
  // Readable
  QT_CREATE,
  QT_DELETE,
  QT_APPEND,
  QT_INSERT,
  QT_UPDATE,
  QT_READ,
  QT_TAKE,

  // Binary
  QT_BCREATE,
  QT_BDELETE,
  QT_BAPPEND,
  QT_BINSERT,
  QT_BUPDATE,
  QT_BREAD,
  QT_BTAKE,
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
};
