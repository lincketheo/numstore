#pragma once

#include "core/ds/strings.h"
#include "core/intf/types.h"
#include "numstore/query/queries/insert.h"

#include "compiler/value/value.h"

typedef struct
{
  // Has type "Expected"
  string vname; // The variable name
  value val;    // either ([] Expected) or (Expected)
  b_size start; // Starting index to insert data into
  bool got_vname;
  bool got_val;
  bool got_start;
} insert_builder;

insert_builder inb_create (void);

err_t inb_accept_string (insert_builder *a, const string vname, error *e);
err_t inb_accept_value (insert_builder *a, value v, error *e);
err_t inb_accept_start (insert_builder *a, b_size start, error *e);

err_t inb_build (insert_query *dest, insert_builder *b, error *e);
