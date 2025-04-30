#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "typing.h"

typedef struct type_builder_s type_builder;

typedef struct
{
  type_builder *stack;
  u32 sp;

  lalloc *type_allocator;  // for allocating return types onto
  lalloc *stack_allocator; // for allocating the internal stack
} type_parser;

typedef struct
{
  lalloc *type_allocator;
  lalloc *stack_allocator;
} tp_params;

err_t tp_create (type_parser *dest, tp_params params);
parse_result tp_feed_token (type_parser *tp, token t);
void tp_release (type_parser *t);
