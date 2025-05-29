#pragma once

#include "ast/type/types.h" // type
#include "ds/strings.h"     // string
#include "mm/lalloc.h"      // lalloc

typedef struct query_s query;

typedef struct
{
  string vname; // Variable name to append
  type type;    // Type of [vname]

  lalloc query_allocator;
  u8 _query_space[2048];

  // TODO
} append_query;

void i_log_append (append_query *q);
query append_query_create(append_query *q);
void append_query_reset (append_query *q);
