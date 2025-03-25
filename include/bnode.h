#include <assert.h>

#define PAGE_SIZE 4096

#include "types.h"

typedef u64 child_ptr_t;
typedef u64 data_ptr_t;
typedef u16 offset_t;
typedef u16 keylen_t;
typedef u16 nkeys_t;

#pragma pack(push, 1)
typedef struct {
  nkeys_t nkeys;

  // [ ptr1, ptr2, ... ptrn + 1 ]
  // [ ofst1, ofst2, ... ofstn ]
  // [ key1, key2, ... keyn ]
  u8 data[PAGE_SIZE - 2];

#define bnode_assert(b) \
  assert(b);            \
  assert(b->nkeys > 0)

#define nptrs(b) ((b)->nkeys + 1)
#define noffsets(b) ((b)->nkeys)

} bnode;
#pragma pack(pop)

////////////////// KEY_VALUE
typedef struct {
  keylen_t keylen;
  u8* key;
  data_ptr_t ptr;

#define bnode_key_value_assert(b) \
  assert(b);                      \
  assert((b)->key);               \
  assert((b)->keylen > 0);        \
  assert((b)->ptr > 0);

} bnode_key_value;

// Returns the total size in bytes of [b]
static inline usize bnode_key_value_size(bnode_key_value* b)
{
  bnode_key_value_assert(b);
  return b->keylen + sizeof(data_ptr_t) + sizeof(keylen_t);
}

// Serializes [k] into [dest]. Assumes [dest] is big enough
static inline void bnode_key_value_serialize(u8* dest, bnode_key_value* k)
{
  bnode_key_value_assert(k);
  u8* head = dest;

  memcpy(dest, &k->keylen, sizeof(keylen_t));
  dest += sizeof(keylen_t);

  memcpy(dest, k->key, k->keylen);
  dest += k->keylen;

  memcpy(dest, &k->ptr, sizeof(data_ptr_t));
}

////////////////// GETTERS
// Gets the start of the pointers array
static inline child_ptr_t* bnode_ptrs(const bnode* b)
{
  bnode_assert(b);

  return (child_ptr_t*)b->data;
}

// Gets the start of the offset array
static inline offset_t* bnode_offsets(const bnode* b)
{
  bnode_assert(b);

  child_ptr_t* child_ptrs_tail = bnode_ptrs(b) + nptrs(b);

  return (offset_t*)child_ptrs_tail;
}

// Gets the start of the keys array
static inline u8* bnode_keys(const bnode* b)
{
  bnode_assert(b);

  offset_t* offsets_tail = bnode_offsets(b) + b->nkeys;

  return (u8*)offsets_tail;
}

// Get the size of the entire bnode
static inline usize bnode_size(bnode* b)
{
  bnode_assert(b);

  // Last offset is the size
  offset_t offset = bnode_offsets(b)[b->nkeys - 1];
  usize before = bnode_keys(b) - b->data;

  return before + offset;
}

// Get the start of key, value at [idx]
static inline u8* bnode_key(bnode* b, usize idx)
{
  bnode_assert(b);
  assert(idx < b->nkeys);

  if (idx == 0) {
    return bnode_keys(b);
  }
  offset_t* offsets = bnode_offsets(b);

  return bnode_keys(b) + offsets[idx - 1];
}

static inline bnode_key_value bnode_get_key_value(bnode* b, usize idx)
{
  assert(b);
  assert(idx < b->nkeys);

  bnode_key_value ret;

  u8* head = bnode_key(b, idx);
  ret.keylen = *(keylen_t*)(head);
  ret.key = head + sizeof(keylen_t);
  ret.ptr = *(data_ptr_t*)(head + sizeof(keylen_t) + ret.keylen);

  bnode_key_value_assert(&ret);

  return ret;
}

static inline bnode_key_value bnode_set_key_value(bnode* b, usize idx)
{
  assert(b);
  assert(idx < b->nkeys);

  bnode_key_value ret;

  u8* head = bnode_key(b, idx);
  ret.keylen = *(keylen_t*)(head);
  ret.key = head + sizeof(keylen_t);
  ret.ptr = *(data_ptr_t*)(head + sizeof(keylen_t) + ret.keylen);

  bnode_key_value_assert(&ret);

  return ret;
}

////////////////// CONSTRUCTOR
bnode bnode_create(bnode_key_value k0, child_ptr_t left, child_ptr_t right)
{
  bnode ret;
  ret.nkeys = 1;

  child_ptr_t* ptrs = bnode_ptrs(&ret);
  ptrs[0] = left;
  ptrs[1] = right;

  offset_t* offsets = bnode_offsets(&ret);
  offsets[0] = bnode_key_value_size(&k0);

  u8* keys = bnode_keys(&ret);
  bnode_key_value_serialize(keys, &k0);

  return ret;
}

void bnode_append_key_value(
    bnode* dest,
    const bnode* src,
    child_ptr_t ptr,
    bnode_key_value k);
