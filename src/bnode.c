#include "bnode.h"
#include "testing.h"

void bnode_append_key_value(
    bnode* dest,
    const bnode* src,
    child_ptr_t ptr,
    bnode_key_value k)
{
  bnode_assert(dest);
  bnode_assert(src);
  bnode_key_value_assert(&k);

  dest->nkeys = src->nkeys + 1;
  offset_t last_offset = bnode_offsets(src)[noffsets(src) - 1];

  // Copy pointers
  child_ptr_t* dest_ptrs = bnode_ptrs(dest);
  const child_ptr_t* src_ptrs = bnode_ptrs(src);
  memcpy(dest_ptrs, src_ptrs, nptrs(src) * sizeof *dest_ptrs);
  dest_ptrs[nptrs(src)] = ptr;

  // Copy offsets
  offset_t* dest_offsets = bnode_offsets(dest);
  offset_t* src_offsets = bnode_offsets(src);
  memcpy(dest_offsets, src_offsets, noffsets(src) * sizeof *dest_offsets);
  dest_offsets[noffsets(src)] = last_offset + bnode_key_value_size(&k);

  // Copy keys
  u8* dest_keys = bnode_keys(dest);
  u8* src_keys = bnode_keys(src);
  memcpy(dest_keys, src_keys, last_offset);
  bnode_key_value_serialize(&dest_keys[last_offset], &k);
}

TEST(foobar)
{
  const char* key0 = "foobar";
  const char* key1 = "foobar";
  const char* key2 = "foobar";
  const char* key3 = "foobar";

#define bnode_key_value_from(cstr, _ptr) \
  (bnode_key_value)                      \
  {                                      \
    .keylen = strlen(cstr),              \
    .key = (u8*)cstr,                    \
    .ptr = _ptr,                         \
  }

  bnode_key_value k0 = bnode_key_value_from(key0, 0);
  bnode_key_value k1 = bnode_key_value_from(key1, 1);
  bnode_key_value k2 = bnode_key_value_from(key2, 2);
  bnode_key_value k3 = bnode_key_value_from(key3, 3);

  bnode src = bnode_create(k0, 4, 5);
}
