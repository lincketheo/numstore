#pragma once

#include "database.h"
#include "navigation.h"
#include "paging.h"
#include "sds.h"
#include "typing.h"
#include "variable.h"
#include "vhash_map.h"

typedef struct
{
  navigator nav;

  struct
  {
    vmeta meta;
    u64 size;
    bool loaded;
  };

  cbuffer *input;       // Place to fill the input data
  cbuffer *output;      // Place to store the output data
  vhash_map *variables; // In memory variable retrieval service
} cursor;

err_t crsr_create (cursor *dest, database *db);
err_t crsr_rewind (cursor *c, const string vname);
err_t crsr_create_variable (cursor *c, const vcreate v);
