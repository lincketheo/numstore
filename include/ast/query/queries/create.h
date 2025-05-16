#pragma once

#include "ast/type/types.h"
#include "ds/strings.h"

typedef struct
{
  string vname;
  type type;
  u8 data[4096]; // for variable string name and type data
} create_query;

void i_log_create (create_query *q);
