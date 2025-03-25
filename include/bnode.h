#include <assert.h>

#define PAGE_SIZE 4096

#include "types.h"

#include <stdio.h>

typedef u64 child_ptr_t;
typedef u64 data_ptr_t;
typedef u16 offset_t;
typedef u16 keylen_t;
typedef u16 nkeys_t;

#pragma pack(push, 1)
typedef struct
{
  nkeys_t nkeys;

  // [ ptr1, ptr2, ... ptrn + 1 ]
  // [ ofst1, ofst2, ... ofstn ]
  // [ key1, key2, ... keyn ]
  u8 data[PAGE_SIZE - 2];

#define bnode_assert(b)                                                       \
  assert (b);                                                                 \
  assert (b->nkeys > 0)

#define nptrs(b) ((b)->nkeys + 1)
#define noffsets(b) ((b)->nkeys)

} bnode;
#pragma pack(pop)

typedef struct
{
  keylen_t keylen;
  u8 *key;
  data_ptr_t ptr;

#define bnode_key_value_assert(b)                                             \
  assert (b);                                                                 \
  assert ((b)->key);                                                          \
  assert ((b)->keylen > 0);                                                   \
  assert ((b)->ptr > 0);

} bnode_key_value;

// Returns the total size in bytes of [b]
usize bnode_key_value_size (bnode_key_value *b);

bnode bnode_create (bnode_key_value k0, child_ptr_t left, child_ptr_t right);

void bnode_push_key_value (bnode *dest, const bnode *src, child_ptr_t ptr,
                           bnode_key_value k);
