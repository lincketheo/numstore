#pragma once

#include "dev/assert.h"
#include "ds/strings.h"
#include "typing.h"

////////////////////////////// A Single Variable
typedef struct
{
  const string vname;
  const type type;
} vcreate;

typedef struct
{
  u64 pgn0;
  const type type;
} vmeta;
