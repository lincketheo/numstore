#pragma once

#include "ds/strings.h"
#include "mm/lalloc.h"

typedef struct
{
  string vname;           // Variable name to delete
  char _vname[512];       // Backing for [vname]
  lalloc vname_allocator; // Allocates the variable name
} delete_query;

void i_log_delete (delete_query *q);
void delete_query_create (delete_query *q);
void delete_query_reset (delete_query *q);
