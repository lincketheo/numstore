#pragma once

#include "compiler/stack_parser/query_parser.h"
#include "compiler/stack_parser/type_parser.h"

#include "compiler/tokens.h" // token
#include "type/types.h"      // u32

/**
 * These are "non terminals" (not exactly, but similar)
 * All parsers visible to ast_parser should have an entry
 */
typedef enum
{
  SBBT_TYPE,
  SBBT_QUERY,
} sb_build_type;

/**
 * Abstract thing that we can build on the stack
 */
typedef struct
{
  sb_build_type type;

  union
  {
    type_parser tb;
    query_parser qb;
  };

} ast_parser;

/**
 * A result of calling build
 */
typedef struct
{
  sb_build_type type;

  union
  {
    type t;
    query q;
  };
} ast_result;

typedef struct
{
  ast_parser *stack;
  u32 sp;
  u32 cap;

  lalloc *type_allocator;  // For allocating types onto
  lalloc *stack_allocator; // For growing the stack
} stack_parser;

typedef struct
{
  lalloc *type_allocator;
  lalloc *stack_allocator;
} sp_params;

/**
 * Returns:
 *   - ERR_NOMEM if there's not enough memory to create the stack
 */
err_t stackp_create (stack_parser *dest, sp_params params, error *e);

stackp_result stackp_feed_token (stack_parser *sp, token t);

void stackp_begin (stack_parser *sp, sb_build_type type);

ast_result stackp_get (stack_parser *sp);
