#pragma once

#include "compiler/stack_parser/query_builder.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "typing.h"

typedef enum
{
  SBBT_TYPE,
  SBBT_QUERY,
} sb_build_type;

typedef struct
{
  sb_build_type type;

  union
  {
    type_builder tb;
    query_builder qb;
  };

} ast_builder;

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

void stackp_push (stack_parser *sp, sb_build_type type);

ast_result stackp_pop (stack_parser *sp);
