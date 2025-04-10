#pragma once

#include "common/macros.h"
#include "common/types.h"
#include "dev/assert.h"


//////////////////////////////////// A Key Value Data Structure

typedef struct
{
  keylen_t keylen;
  char* key;
  page_ptr ptr;
} bnode_kv;

static inline int bnode_kv_valid(const bnode_kv* p)
{
  return p->key != NULL && p->keylen > 0;
}

DEFINE_ASSERT(bnode_kv, bnode_kv)

u64 bnode_kv_size(const bnode_kv* b);

//////////////////////////////////// A B-Node Data Structure

/**
 * BNode Axioms
 * 1. A BNode will always be able to hold two or more key values
 *   - (restrict key len - PAGE_SIZE / 4 - TODO - this is best guess)
 */
#define MAX_KEYLEN PAGE_SIZE / 4
#pragma pack(push, 1)
typedef struct
{
  nkeys_t nkeys;
  u8 isleaf;

  // Data is 2 * PAGE_SIZE because various operations
  // will overflow first, then correct (btree insert)
  // Routines must garuntee that they don't leave a node
  // at an invalid state
  u8 data[2 * PAGE_SIZE - sizeof(nkeys_t) - sizeof(u8)];
} bnode;
#pragma pack(pop)

u64 bnode_size(const bnode* b);

static inline int bnode_valid(const bnode* p)
{
  return p->nkeys > 0 && bnode_size(p) <= PAGE_SIZE;
}

DEFINE_ASSERT(bnode, bnode)

int bnode_insert_kv(bnode* dest, const bnode* src, bnode_kv* k);

int bnode_find_kv(const bnode* b, bnode_kv* k, u64* idx);




