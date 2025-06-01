#pragma once

#include "ast/type/types.h" // type
#include "ds/strings.h"     // string
#include "mm/lalloc.h"      // lalloc

typedef struct query_s query;

typedef struct
{
  string vname; // Variable name to create
  type type;    // Type of [vname]

  /**
   * Space to store variable name and
   * dynamically allocated type
   */
  lalloc query_allocator;
  u8 _query_space[2048];
} create_query;

void i_log_create (create_query *q);
query create_query_create (create_query *q);
void create_query_reset (create_query *q);
