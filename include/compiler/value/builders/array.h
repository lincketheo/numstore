#pragma once

#include "compiler/value/value.h"        // value
#include "compiler/value/values/array.h" // array
#include "core/ds/llist.h"               // llnode
#include "core/mm/lalloc.h"              // lalloc

typedef struct
{
  value v;
  llnode link;
} array_llnode;

typedef struct
{
  llnode *head;

  lalloc *work;
  lalloc *dest;
} array_builder;

array_builder arb_create (lalloc *work, lalloc *dest);
err_t arb_accept_value (array_builder *o, value v, error *e);
err_t arb_build (array *dest, array_builder *b, error *e);
