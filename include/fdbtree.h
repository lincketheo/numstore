#pragma once

#include "bnode.h"
#include "page_alloc.h"

typedef struct {
  page_alloc alloc;
} fdbtree;

#define fdbtree_assert(b) assert(b)

fdbtree fdbtree_create(bnode_kv k0, int fd);
