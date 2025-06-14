#pragma once

#include "ast/query/queries/create.h" // create_query
#include "ast/type/types.h"           // type
#include "ds/strings.h"               // string

typedef struct
{
  string vname;
  type type;
  bool got_type;
  bool got_vname;
} create_builder;

create_builder crb_create (void);

err_t crb_accept_string (create_builder *b, const string vname, error *e);
err_t crb_accept_type (create_builder *b, type t, error *e);

err_t crb_build (create_query *dest, create_builder *b, error *e);
