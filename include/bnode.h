#include <assert.h>

#define PAGE_SIZE 4096

#include "types.h"

#include <stdio.h>

/**
  * A B-Tree Node (Note, not a B+Tree) is layed out in the following format:
  * [nkeys] (2 bytes)
  * [cptr1, cptr2, ... cptr(nkeys + 1)] (nkeys + 1) * 2 bytes
  * [offst1, offst2, ... offst(nkeys)] (nkeys) * 2 bytes
  * [n key dptr, n key dptr ...]
  *
  * Where cptr is a pointer to a child node and
  * dptr is a pointer to the data node start
  */
typedef u64 child_ptr_t;
typedef u64 data_ptr_t;
typedef u16 offset_t;
typedef u16 keylen_t;
typedef u16 nkeys_t;

//////////////////////////////////// A B-Node Data Structure

#pragma pack(push, 1)
typedef struct
{
  nkeys_t nkeys;
  u8 data[PAGE_SIZE - sizeof(nkeys_t)];
} bnode;
#pragma pack(pop)

#define bnode_assert(b)                                                       \
assert (b);                                                                 \
assert (b->nkeys > 0)

//////////////////////////////////// A Key Value Data Structure

typedef struct
{
  keylen_t keylen;
  char *key;
  data_ptr_t ptr;
} bnode_kv;

#define bnode_kv_assert(b)                                             \
assert (b);                                                                 \
assert ((b)->key);                                                          \
assert ((b)->keylen > 0);                                                   \

//////////////////////////////////// BTree

typedef struct {
  int fd;
} btree;

btree btree_create(bnode_kv k0, int fd);

void btree_insert(btree* b, const bnode_kv k);

int btree_fetch(btree* b, bnode_kv *k);
