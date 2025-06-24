#pragma once

#include "core/ds/strings.h"   // string
#include "core/errors/error.h" // error

#include "compiler/ast/query/queries/delete.h" // delete_query
#include "compiler/ast/type/types.h"           // type

typedef struct
{
  string vname;
  bool got_vname;
} delete_builder;

delete_builder dltb_create (void);

err_t dltb_accept_string (delete_builder *b, const string vname, error *e);

err_t dltb_build (delete_query *dest, delete_builder *b, error *e);
