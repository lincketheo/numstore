#pragma once

#include "compiler/tokens.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "typing.h"

typedef struct type_bldr_s type_bldr;

typedef enum
{

  TPR_EXPECT_NEXT_TOKEN,
  TPR_EXPECT_NEXT_TYPE,
  TPR_MALLOC_ERROR,
  TPR_SYNTAX_ERROR,
  TPR_DONE,

} tp_result;

typedef struct
{
  type_bldr *stack;
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
tp_result tp_feed_token (type_parser *tp, token t);
void tp_release (type_parser *t);
