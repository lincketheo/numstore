#include "bnode.h"
#include "testing.h"

////////////////// KEY_VALUE
// Returns the total size in bytes of [b]
usize
bnode_key_value_size (bnode_key_value *b)
{
  bnode_key_value_assert (b);
  return b->keylen + sizeof (data_ptr_t) + sizeof (keylen_t);
}

// Serializes [k] into [dest]. Assumes [dest] is big enough
static inline void
bnode_key_value_serialize (u8 *dest, bnode_key_value *k)
{
  bnode_key_value_assert (k);

  memcpy (dest, &k->keylen, sizeof (keylen_t));
  dest += sizeof (keylen_t);

  memcpy (dest, k->key, k->keylen);
  dest += k->keylen;

  memcpy (dest, &k->ptr, sizeof (data_ptr_t));
}

////////////////// GETTERS
// Gets the start of the pointers array
static inline child_ptr_t *
bnode_ptrs (const bnode *b)
{
  bnode_assert (b);

  return (child_ptr_t *)b->data;
}

// Gets the start of the offset array
static inline offset_t *
bnode_offsets (const bnode *b)
{
  bnode_assert (b);

  child_ptr_t *child_ptrs_tail = bnode_ptrs (b) + nptrs (b);

  return (offset_t *)child_ptrs_tail;
}

// Gets the start of the keys array
static inline u8 *
bnode_keys (const bnode *b)
{
  bnode_assert (b);

  offset_t *offsets_tail = bnode_offsets (b) + b->nkeys;

  return (u8 *)offsets_tail;
}

// Get the size of the entire bnode
static inline usize
bnode_size (bnode *b)
{
  bnode_assert (b);

  // Last offset is the size
  offset_t offset = bnode_offsets (b)[b->nkeys - 1];
  usize before = bnode_keys (b) - b->data;

  return before + offset;
}

// Get the start of key, value at [idx]
static inline u8 *
bnode_key_value_start (bnode *b, usize idx)
{
  bnode_assert (b);
  assert (idx < b->nkeys);

  if (idx == 0)
    {
      return bnode_keys (b);
    }
  offset_t *offsets = bnode_offsets (b);

  return bnode_keys (b) + offsets[idx - 1];
}

static inline bnode_key_value
bnode_get_key_value (bnode *b, usize idx)
{
  assert (b);
  assert (idx < b->nkeys);

  bnode_key_value ret;

  u8 *head = bnode_key_value_start (b, idx);
  ret.keylen = *(keylen_t *)(head);
  ret.key = head + sizeof (keylen_t);
  ret.ptr = *(data_ptr_t *)(head + sizeof (keylen_t) + ret.keylen);

  bnode_key_value_assert (&ret);

  return ret;
}

static inline void
bnode_key_value_print (FILE *ofp, bnode_key_value *b)
{
  assert (ofp);
  bnode_key_value_assert (b);

  fprintf (ofp, "(%.*s, %llu)", b->keylen, b->key, b->ptr);
}

////////////////// CONSTRUCTOR

bnode
bnode_create (bnode_key_value k0, child_ptr_t left, child_ptr_t right)
{
  bnode ret;
  ret.nkeys = 1;

  child_ptr_t *ptrs = bnode_ptrs (&ret);
  ptrs[0] = left;
  ptrs[1] = right;

  offset_t *offsets = bnode_offsets (&ret);
  offsets[0] = bnode_key_value_size (&k0);

  u8 *keys = bnode_keys (&ret);
  bnode_key_value_serialize (keys, &k0);

  return ret;
}

static void
bnode_append_key_value (bnode *dest, const bnode *src, child_ptr_t ptr,
                        bnode_key_value k)
{
  bnode_assert (src);
  bnode_key_value_assert (&k);

  dest->nkeys = src->nkeys + 1;
  offset_t last_offset = bnode_offsets (src)[noffsets (src) - 1];

  // Copy pointers
  child_ptr_t *dest_ptrs = bnode_ptrs (dest);
  const child_ptr_t *src_ptrs = bnode_ptrs (src);
  memcpy (dest_ptrs, src_ptrs, nptrs (src) * sizeof *dest_ptrs);
  dest_ptrs[nptrs (src)] = ptr;

  // Copy offsets
  offset_t *dest_offsets = bnode_offsets (dest);
  offset_t *src_offsets = bnode_offsets (src);
  memcpy (dest_offsets, src_offsets, noffsets (src) * sizeof *dest_offsets);
  dest_offsets[noffsets (src)] = last_offset + bnode_key_value_size (&k);

  // Copy keys
  u8 *dest_keys = bnode_keys (dest);
  u8 *src_keys = bnode_keys (src);
  memcpy (dest_keys, src_keys, last_offset);
  bnode_key_value_serialize (&dest_keys[last_offset], &k);
}

static inline void
bnode_print (FILE *ofp, bnode *b)
{
  bnode_assert (b);

  child_ptr_t *ptrs = bnode_ptrs (b);

  // Print Nodes
  fprintf (ofp, "%llu ", ptrs[0]);
  for (int i = 0; i < b->nkeys; ++i)
    {
      bnode_key_value key = bnode_get_key_value (b, i);
      bnode_key_value_print (ofp, &key);
      fprintf (ofp, " %llu ", ptrs[i + 1]);
    }
  fprintf (ofp, "\n");
}

TEST (foobar)
{
  const char *key0 = "foobar";
  const char *key1 = "fizb";
  const char *key2 = "bazbi";
  const char *key3 = "barbuz";

#define bnode_key_value_from(cstr, _ptr)                                      \
  (bnode_key_value)                                                           \
  {                                                                           \
    .keylen = strlen (cstr), .key = (u8 *)cstr, .ptr = _ptr,                  \
  }
  bnode_key_value k0 = bnode_key_value_from (key0, 1);
  bnode_key_value k1 = bnode_key_value_from (key1, 2);
  bnode_key_value k2 = bnode_key_value_from (key2, 3);
  bnode_key_value k3 = bnode_key_value_from (key3, 4);
#undef bnode_key_value_from

  bnode b0 = bnode_create (k0, 4, 5);
  bnode b1;
  bnode_append_key_value (&b1, &b0, 9, k1);
  bnode_append_key_value (&b0, &b1, 11, k2);
  bnode_append_key_value (&b1, &b0, 14, k3);

  bnode_key_value k0a = bnode_get_key_value (&b1, 0);
  bnode_key_value k1a = bnode_get_key_value (&b1, 1);
  bnode_key_value k2a = bnode_get_key_value (&b1, 2);
  bnode_key_value k3a = bnode_get_key_value (&b1, 3);

#define test_assert_bnode_key_value_equal(_k0, _k1)                           \
  test_assert_equal (_k0.keylen, _k1.keylen, "%du");                          \
  test_assert_equal (strncmp ((char *)_k0.key, (char *)_k1.key, _k0.keylen),  \
                     0, "%d");                                                \
  test_assert_equal (_k0.ptr, _k1.ptr, "%llu");

  test_assert_bnode_key_value_equal (k0a, k0);
  test_assert_bnode_key_value_equal (k1a, k1);
  test_assert_bnode_key_value_equal (k2a, k2);
  test_assert_bnode_key_value_equal (k3a, k3);
#undef test_assert_bnode_key_value_equal
}

/**
i16
bnode_get_key_pos (const bnode *src, child_ptr_t ptr, bnode_key_value k)
{
  bnode_assert (src);
  for(int i = 0; i < src->nkeys; ++i) {

    strncmp();
  }
}
*/

void
bnode_push_key_value (bnode *dest, const bnode *src, child_ptr_t ptr,
                      bnode_key_value k)
{
  bnode_assert (src);
  bnode_key_value_assert (&k);
}
