#pragma once

#include "ast/type/types.h"
#include "ds/strings.h"

typedef struct
{
  string vname;
  type type;
  lalloc alloc;  // Allocates all the query stuff
  u8 data[4096]; // for variable string name and type data
} create_query;

void i_log_create (create_query *q);
void create_query_create (create_query *q);
void create_query_reset (create_query *q);
