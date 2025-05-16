#pragma once

#include "ds/strings.h"
#include "mm/lalloc.h"

typedef struct
{
  string vname;
  u8 data[512]; // Variable name
} delete_query;

void i_log_delete (delete_query *q);
