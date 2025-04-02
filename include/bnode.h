#pragma once

#include "types.h"

#include <assert.h>



//////////////////////////////////// A Key Value Data Structure

typedef struct
{
  keylen_t keylen;
  char* key;
  page_ptr ptr;
} bnode_kv;

#define bnode_kv_assert(b) \
  assert(b);               \
  assert((b)->key);        \
  assert((b)->keylen > 0);

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

  // Data is 2 * PAGE_SIZE because various operations
  // will overflow first, then correct (btree insert)
  // Routines must garuntee that they don't leave a node
  // at an invalid state
  u8 data[2 * PAGE_SIZE - sizeof(nkeys_t)];
} bnode;
#pragma pack(pop)

#define bnode_assert(b) \
  assert(b);            \
  assert(b->nkeys > 0); \
  assert(bnode_size(b) <= PAGE_SIZE)

u64 bnode_size(const bnode* b);

void bnode_split_part_1(
  bnode* left, 
  bnode* center, 
  bnode* right, 
  const bnode* src);

void bnode_split_part_2(
  page_ptr left, 
  bnode* center, 
  page_ptr right);

int bnode_is_leaf(const bnode* b);

int bnode_insert_kv(bnode* dest, const bnode* src, bnode_kv* k);

int bnode_find_kv(const bnode* b, bnode_kv* k, u64* idx);
