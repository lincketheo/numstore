#pragma once

#include "core/ds/strings.h" // string
#include "core/mm/lalloc.h"  // lalloc

typedef struct
{
  string vname; // Variable to delete
} delete_query;

delete_query delete_query_create (void);
void i_log_delete (delete_query *q);
bool delete_query_equal (const delete_query *left, const delete_query *right);
