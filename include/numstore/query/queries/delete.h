#pragma once

#include "core/ds/strings.h" // string
#include "core/mm/lalloc.h"  // lalloc

typedef struct query_s query;

typedef struct
{
  string vname; // Variable to delete

  /**
   * Space to store variable name
   */
  u8 _query_space[512];
  lalloc query_allocator;
} delete_query;

void i_log_delete (delete_query *q);
query delete_query_create (delete_query *q);
void delete_query_reset (delete_query *q);
bool delete_query_equal (const delete_query *left, const delete_query *right);
