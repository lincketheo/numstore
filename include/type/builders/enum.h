#pragma once

#include "ds/llist.h"        // llnode
#include "ds/strings.h"      // string
#include "errors/error.h"    // err_t
#include "mm/lalloc.h"       // lalloc
#include "type/types/enum.h" // enum_t

typedef struct
{
  string key;
  llnode link;
} k_llnode;

typedef struct
{
  llnode *head;
  lalloc *alloc;
} enum_builder;

enum_builder enb_create (lalloc *alloc);

err_t enb_accept_key (enum_builder *eb, const string key, error *e);

err_t enb_build (enum_t *dest, enum_builder *eb, lalloc *destination, error *e);
