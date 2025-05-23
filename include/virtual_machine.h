#pragma once

#include "ast/query/query.h" // query
#include "ds/cbuffer.h"
#include "errors/error.h" // err_t
#include "paging/pager.h" // pager

typedef struct
{
  cbuffer *query_input;
  cbuffer *output;
} vm;

err_t query_execute (pager *p, query *q, error *e);
