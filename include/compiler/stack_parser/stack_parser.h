#pragma once

#include "compiler/stack_parser/query_parser.h"
#include "compiler/stack_parser/type_parser.h"

#include "compiler/tokens.h" // token
#include "type/types.h"      // u32

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
    type_parser tb;
    query_parser qb;
  };

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

typedef struct
{
  ast_parser stack[20];
  u32 sp;

  lalloc *type_allocator;
} stack_parser;

typedef struct
{
  lalloc *type_allocator;
} sp_params;

stack_parser stackp_create (sp_params params);

stackp_result stackp_feed_token (stack_parser *sp, token t);

void stackp_reset (stack_parser *sp);

void stackp_begin (stack_parser *sp, sb_build_type type);

ast_result stackp_get (stack_parser *sp);
