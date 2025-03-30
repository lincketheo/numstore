#pragma once

#include "bnode.h"

typedef struct {
  int fd;
} fdbtree;

#define fdbtree_assert(b) assert(b)

fdbtree fdbtree_create(bnode_kv k0, int fd);
