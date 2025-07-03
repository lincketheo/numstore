#pragma once

#include "core/ds/strings.h" // string
#include "core/mm/lalloc.h"  // lalloc

#include "numstore/type/types.h" // type

typedef struct
{
  string vname;
  type type;
} create_query;

/**
 * Log create and do nothing
 */
void i_log_create (create_query *q);

/**
 * Deep equality check on two create_queries
 */
bool create_query_equal (const create_query *left, const create_query *right);
