#pragma once

#include "ds/strings.h"
#include "errors/error.h"
#include "query/query.h"
#include "type/types/prim.h"

typedef struct
{
  enum
  {
    STCTRL_EXECTUING, // executing a query - workers are free to work
    STCTRL_ERROR,     // there was an error in this query - discard all data
    STCTRL_WRITING,   // workers shouldn't expect input data
  } state;

  char _strs[2048];
  string strs[50];
  type types[50];
  query query;

  lalloc _strs_alloc;
  lalloc strs_alloc;
  lalloc types_alloc;

  error e;
} stmtctrl;

void stmtctrl_create (stmtctrl *dest);
