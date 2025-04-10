#pragma once

#include "bnode.h"
#include "ns_assert.h"
#include "page_alloc.h"

typedef struct {
  page_alloc alloc;
} fdbtree;

static inline int fdbtree_valid(const fdbtree* f) {
  return page_alloc_valid(&f->alloc);
}

DEFINE_ASSERT(fdbtree, fdbtree)

int fdbtree_open(fdbtree* dest, bnode_kv k0, const char* fname);
