#pragma once

#include "ds/strings.h"
#include "type/types/enum.h"

typedef struct
{
  string *keys;
  u32 len;
  u32 cap;
  lalloc *alloc;
} enum_builder;

err_t enb_create (enum_builder *dest, lalloc *alloc, error *e);
err_t enb_accept_key (enum_builder *eb, const string key, error *e);
err_t enb_build (enum_t *dest, enum_builder *eb, error *e);
