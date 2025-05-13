#pragma once

#include "ds/strings.h"           // string
#include "errors/error.h"         // error
#include "query/queries/delete.h" // delete_query
#include "type/types.h"           // type

typedef struct
{
  string vname;
  bool got_vname;
} delete_builder;

delete_builder dltb_create (void);
err_t dltb_accept_string (delete_builder *b, const string vname, error *e);
err_t dltb_build (delete_query *dest, delete_builder *b, error *e);
