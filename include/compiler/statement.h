#pragma once

#include "numstore/query/query.h"

typedef struct
{
  query q;

  lalloc qspace;
  u8 query_space[2048];
} statement;

statement *statement_create (error *e);
