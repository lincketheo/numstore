#pragma once

#include "dev/assert.h"
#include "paging.h"
#include "sds.h"
#include "types.h"
#include "typing.h"
#include "variable.h"

typedef enum
{
  QT_CREATE,
  QT_WRITE,
  QT_READ,
} query_t;

//////////////////////////////// CREATE

typedef struct
{
  const string vname;
  const type *type;
  pager *p;
} create_op;

DEFINE_DBG_ASSERT_H (create_op, create_op, c);

void create_execute (create_op *c);

//////////////////////////////// WRITE

typedef struct
{
  u32 len;
  struct
  {
    u32 len;
    const string *vnames;
  } * seq;
} wfmt;

typedef struct
{
  u32 n;
  wfmt fmt;
} write_op;

DEFINE_DBG_ASSERT_H (write_op, write_op, w);

//////////////////////////////// OPERATION

typedef struct
{

  query_t type;

  union
  {
    create_op create;
    write_op write;
  };
} query_plan;
