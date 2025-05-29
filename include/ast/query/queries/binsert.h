#pragma once

#include "ast/type/types.h" // type
#include "ds/strings.h"     // string
#include "mm/lalloc.h"      // lalloc

typedef struct query_s query;

typedef struct
{
  lalloc query_allocator;
  u8 _query_space[2048];

  // TODO
} binsert_query;

void i_log_binsert (binsert_query *q);
query binsert_query_create(binsert_query *q);
void binsert_query_reset (binsert_query *q);
