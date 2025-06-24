#pragma once

#include "core/ds/strings.h" // TODO

#include "compiler/ast/value/value.h" // TODO

typedef struct
{
  string keys;
  value value;

  bool is_key_chosen;
  bool is_value_chosen;
} unionv_builder;

err_t unvb_create (unionv_builder *dest, lalloc *alloc);
err_t unvb_accept_key (unionv_builder *eb, string key);
err_t unvb_accept_value (unionv_builder *eb, value v);
err_t unvb_build (union_v *dest, unionv_builder *eb);
