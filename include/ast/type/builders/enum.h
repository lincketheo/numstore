#pragma once

#include "ast/type/types/enum.h" // enum_t
#include "ds/llist.h"            // llnode
#include "ds/strings.h"          // string
#include "errors/error.h"        // err_t
#include "mm/lalloc.h"           // lalloc

typedef struct
{
  string key;
  llnode link;
} k_llnode;

typedef struct
{
  llnode *head;  // Linked list
  lalloc *alloc; // Allocator for working space
  lalloc *dest;  // Destination to allocate to on build
} enum_builder;

enum_builder enb_create (
    lalloc *alloc,
    lalloc *dest);

err_t enb_accept_key (
    enum_builder *eb,
    const string key,
    error *e);

err_t enb_build (
    enum_t *dest,
    enum_builder *eb,
    error *e);
