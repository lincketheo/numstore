#pragma once

#include "query/queries/create.h"
#include "query/queries/delete.h"

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
    create_query cquery;
    delete_query dquery;
  };
} query;
