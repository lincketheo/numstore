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

/**
 * Wraps around all the types you can parse
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

/**
 * Wraps around [create] and [delete]
 */
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

/**
 * Generic wrapper around type and query
 * non terminals
 */
typedef struct
{
  sb_build_type type;

  union
  {
    type_parser tb;
    query_parser qb;
  };

  query cur;       // Current query we're working on
  u32 alloc_start; // The start pos of the allocator to reset on pop

} ast_parser;

typedef struct
{
  sb_build_type type;

  union
  {
    type t;
    query q;
  };
} ast_result;

/**
 * Given your current state and the next token [t], what
 * do you expect next?
 */
sb_feed_t
ast_parser_expect_next (
    ast_parser *b,
    token t);

/**
 * Build the ast parser [b] and stores
 * the results in [dest] if successful
 *
 * Errors:
 *   - SPR_SYNTAX_ERROR (ERR_SYNTAX)
 *   - SPR_MALLOC_ERROR (ERR_NOMEM)
 */
ast_result ast_parser_to_result (ast_parser *b);

/**
 * Gives this parser a type [t]
 *
 * Errors:
 *   - SPR_SYNTAX_ERROR (ERR_SYNTAX)
 *   - SPR_MALLOC_ERROR (ERR_NOMEM)
 */
stackp_result
ast_parser_accept_type (
    ast_parser *b,
    type t,
    error *e);

/**
 * Gives this parser a token [t]
 *
 * Errors:
 *   - SPR_SYNTAX_ERROR (ERR_SYNTAX)
 *   - SPR_MALLOC_ERROR (ERR_NOMEM)
 */
stackp_result
ast_parser_accept_token (
    ast_parser *b,
    token t,
    lalloc *alloc,
    error *e);
