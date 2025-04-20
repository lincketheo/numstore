#pragma once

#include "dev/assert.h"
#include "paging.h"
#include "sds.h"
#include "types.h"
#include "typing.h"

typedef enum
{
  OP_CREATE,
  OP_WRITE,
  OP_READ,
} op_t;

//////////////////////////////// CREATE
typedef struct
{
  const string vname;
  const type *type;
  pager *p;
} create;

DEFINE_DBG_ASSERT_H (create, create, c);
void create_init (create *c, const string vname, const type *type, pager *p);
void create_execute (create *c);

//////////////////////////////// WRITE
typedef struct
{
  u64 page0;
  type *type;
} wvar;

typedef struct
{
  u32 len;
  struct
  {
    u32 len;
    wvar *nbrs;
  } * seq;
} wfmt;

typedef struct
{
  u32 n;
  wfmt fmt;
} write;

//////////////////////////////// READ
typedef struct
{
  u64 page0;
  type_subset *type;
} rvar;

typedef struct
{
  u32 len;
  struct
  {
    u32 len;
    rvar *nbrs;
  } * seq;
} rfmt;

typedef struct
{
  int start;
  int stop;
  int end;
  rfmt fmt;
  cbuffer *input;
} read;

//////////////////////////////// OPERATION
/**
 * An operation is the thing that's passed to the
 * VM. Unlike regular operations, it contains a lot of
 * information in it
 */
typedef struct
{
  u8 type;
  union
  {
    create create;
    read read;
    write write;
  };
} op;
