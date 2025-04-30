#pragma once

#include "query/queries/append.h"
#include "query/queries/create.h"
#include "query/queries/delete.h"
#include "query/queries/insert.h"
#include "query/queries/read.h"
#include "query/queries/take.h"
#include "query/queries/update.h"

typedef enum
{
  QT_CREATE,
  QT_DELETE,
  QT_APPEND,
  QT_INSERT,
  QT_UPDATE,
  QT_READ,
  QT_TAKE,
} query_t;

///// QUERY
typedef struct
{
  query_t type;

  union
  {
    create_args cargs;
    delete_args dargs;
    append_args aargs;
    insert_args iargs;
    read_args rargs;
    take_args targs;
    update_args uargs;
  };
} query;
