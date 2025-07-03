#pragma once

#include "compiler/ast/query.h"

typedef struct
{
  query q;

  // Room for the query variables
  lalloc qspace;
  u8 query_space[2048];
} statement;

statement *statement_create (error *e);
void statement_free (statement *s);
