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
typedef struct
{
  query_t type;
  union
  {
    // Pointer because
    // these are big
    create_query *create;
    delete_query *delete;
  };
  lalloc *qalloc; // Allocator for query data
} query;
