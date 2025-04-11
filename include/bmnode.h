#pragma once

#include "common/macros.h"
#include "common/types.h"

typedef struct {
  u8* page; // A pointer to the page head

  nkeys_t* nkeys;     // Pointer to the number of keys
  page_ptr* children; // Pointer to child node ptrs
  offset_t* offsets;  // Pointer to key value offsets
} bmnode;

typedef struct {
  keylen_t* keylen; // Len of key
  char* key;        // The key
  page_ptr* data;   // The head of the data
} bmnode_kv;

static inline int bmnode_valid(const bmnode* b)
{
  int ret = b != NULL;
  ret = ret && b->page;
  ret = ret && b->nkeys;
  ret = ret && b->children;
  ret = ret && b->offsets;
  ret = ret && (*b->nkeys > 0);
  return ret;
}

static inline int bmnode_kv_valid(const bmnode_kv* b)
{
  return b && b->keylen && b->key && b->data && *b->keylen > 0;
}

DEFINE_ASSERT(bmnode_kv, bmnode_kv)

DEFINE_ASSERT(bmnode, bmnode)

static inline void bmnode_init(bmnode* b)
{
  b->nkeys = (nkeys_t*)&b->page[0];
  b->children = (page_ptr*)(b->nkeys + 1);
  b->offsets = (offset_t*)(b->children + *b->nkeys + 1);
  bmnode_assert(b);
}

static inline bmnode_kv bmnode_kv_create(u8* head)
{
  bmnode_kv ret;
  ret.keylen = (keylen_t*)head;
  ret.key = (char*)(ret.keylen + 1);
  ret.data = (page_ptr*)(ret.key + *ret.keylen);
  bmnode_kv_assert(&ret);
  return ret;
}

/**
 * Other things you could do:
 * 1. Mod Log
 * 2. Variable Length keys - key map / indirection
 * 3. Prefix Compression (store prefix at start)
 * 4. Deduplication
 * 5. Bulk create write, then build from bottom to top
 */

bmnode bmnode_create(
    u8* page,
    char* key,
    keylen_t keylen,
    page_ptr data);

typedef enum {
  BMNIR_DUPLICATE,
  BMNIR_SUCCESS,
  BMNIR_FULL,
} bmnir;

// Returns 1 on success 0 if full
bmnir bmnode_insert_kv(
    bmnode* dest,
    const char* key,
    keylen_t keylen,
    page_ptr data);

// Returns 1 on success 0 if not found
// On 0 idx is set as index of necessary child
int bmnode_find_key(
    const bmnode* b,
    const char* key,
    keylen_t keylen,
    u32* idx);

bmnode_kv bmnode_get_kv(const bmnode* b, u32 idx);
