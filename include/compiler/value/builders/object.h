#pragma once

#include "compiler/value/value.h"         // value
#include "compiler/value/values/object.h" // array
#include "core/ds/llist.h"                // llnode
#include "core/ds/strings.h"              // string
#include "core/mm/lalloc.h"               // lalloc

typedef struct
{
  string key;
  value v;
  llnode link;
} object_llnode;

typedef struct
{
  llnode *head;

  u16 klen;
  u16 tlen;

  lalloc *work;
  lalloc *dest;
} object_builder;

object_builder objb_create (lalloc *work, lalloc *dest);
err_t objb_accept_string (object_builder *o, const string key, error *e);
err_t objb_accept_value (object_builder *o, value v, error *e);
err_t objb_build (object *dest, object_builder *b, error *e);
