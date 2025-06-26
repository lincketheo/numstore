#pragma once

#include "compiler/value/value.h"

#include "core/intf/types.h"

typedef struct query_s query;

typedef struct
{
  string vname;
  value val;
  b_size start;

  lalloc query_allocator;
  u8 _query_space[2048];
} insert_query;

void i_log_insert (insert_query *q);
query insert_query_create (insert_query *q);
void insert_query_reset (insert_query *q);
bool insert_query_equal (const insert_query *left, const insert_query *right);
