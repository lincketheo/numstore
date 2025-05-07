#pragma once

#include "dev/assert.h"
#include "ds/strings.h"
#include "type/types.h"

////////////////////////////// A Single Variable
typedef struct
{
  pgno pgn0; // Root page number
  type type; // Variable Type
} vmeta;

typedef struct
{
  string vstr; // The variable string
  string tstr; // The type string (to be deserialized)
  pgno pg0;    // The starting page
} var_hash_entry;
