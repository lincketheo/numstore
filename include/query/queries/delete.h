#pragma once

#include "ds/strings.h"

typedef struct
{
  string vname;
} delete_query;

void i_log_delete (delete_query *q);
