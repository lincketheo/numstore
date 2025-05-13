#pragma once

#include "compiler/stack_parser/types/enum.h"
#include "compiler/stack_parser/types/kvt.h"
#include "compiler/stack_parser/types/sarray.h"

#include "compiler/stack_parser/queries/create.h"
#include "compiler/stack_parser/queries/delete.h"
#include "compiler/tokens.h"
#include "query/query.h"

////////////////// TYPE BUILDER
/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> [NUM] A | [NUM]
 * K -> \eps | , T K
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * I = ENUM_KEY_LIST
 */

typedef struct
{

  enum
  {
    TB_UNKNOWN,

    TB_STRUCT,
    TB_UNION,
    TB_ENUM,
    TB_PRIM,
    TB_SARRAY,

  } state;

  union
  {
    kvt_parser kvp;
    enum_parser enp;
    sarray_parser sap;
  };

  type ret;
} type_parser;

typedef struct
{
  enum
  {
    QP_UNKNOWN,

    QP_CREATE,
    QP_DELETE,

  } state;

  union
  {
    create_parser cp;
    delete_parser dp;
  };

  query ret;
} query_parser;

typedef struct
{
  sb_build_type type;

  union
  {
    type_parser tb;
    query_parser qb;
  };
} ast_parser;
