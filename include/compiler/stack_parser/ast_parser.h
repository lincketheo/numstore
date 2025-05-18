#pragma once

// Type Impls
#include "compiler/stack_parser/types/enum.h"
#include "compiler/stack_parser/types/kvt.h"
#include "compiler/stack_parser/types/sarray.h"

// Query Impls
#include "compiler/stack_parser/queries/create.h"
#include "compiler/stack_parser/queries/delete.h"

#include "ast/query/query.h" // query
#include "ast/type/types.h"  // type
#include "compiler/tokens.h" // token

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
    prim_t p;
    kvt_parser kvp;
    enum_parser enp;
    sarray_parser sap;
  };
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
} query_parser;

typedef struct
{
  sb_build_type type;

  union
  {
    type_parser tb;
    query_parser qb;
  };
  query cur;
  u32 alloc_start;
} ast_parser;
