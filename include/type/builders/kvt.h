#pragma once

#include "ds/llist.h"   // llnode
#include "ds/strings.h" // string
#include "intf/types.h" // u32
#include "mm/lalloc.h"  // lalloc
#include "type/types.h" // type

typedef struct
{
  string key;
  type value;
  llnode link;
} kv_llnode;

typedef struct
{
  llnode *head;

  u16 klen;
  u16 tlen;

  lalloc *alloc; // For growing types and keys arrays
} kvt_builder;

kvt_builder kvb_create (lalloc *alloc);

err_t kvb_accept_key (kvt_builder *eb, string key, error *e);
err_t kvb_accept_type (kvt_builder *eb, type t, error *e);

err_t kvb_union_t_build (union_t *dest, kvt_builder *eb, lalloc *destination, error *e);
err_t kvb_struct_t_build (struct_t *dest, kvt_builder *eb, lalloc *destination, error *e);
