#pragma once

#include "compiler/stack_parser/query_builder.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "services/services.h"
#include "typing.h"

/**
 * These are "non terminals" (not exactly, but similar)
 * All builders visible to ast_builder should have an entry
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
    type_builder tb;
    query_builder qb;
  };

} ast_builder;

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
  ast_builder *stack;
  u32 sp;
  u32 cap;

  lalloc *type_allocator;  // for allocating return types onto
  lalloc *stack_allocator; // for allocating the internal stack
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
err_t stackp_create (stack_parser *dest, sp_params params);

stackp_result stackp_feed_token (stack_parser *sp, token t);

void stackp_begin (stack_parser *sp, sb_build_type type);

ast_result stackp_get (stack_parser *sp);
