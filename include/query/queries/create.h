#pragma once

#include "ds/strings.h"
#include "type/types.h"

typedef struct
{
  string vname;
  type type;
} create_query;

void i_log_create (create_query *q);
