#include "bmnode.h"
#include "common/macros.h"
#include "common/types.h"
#include "dev/assert.h"
#include "dev/testing.h"

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

/////////////////////////////// HELPERS
static inline offset_t
bmnode_prefix_tail (bmnode *b)
{
  u8 *endptr = (u8 *)&b->offsets[*b->nkeys];
  assert (endptr > b->data);
  return endptr - b->data;
}

static inline offset_t
bmnode_min_offset (bmnode *b)
{
  bmnode_assert (b);
  offset_t ret = PAGE_SIZE;
  for (int i = 0; i < *b->nkeys; ++i)
    {
      if (b->offsets[i] < ret)
        {
          ret = b->offsets[i];
        }
    }
  return ret;
}

static inline void
unsafe_bmnode_expand_and_split (bmnode *b, u32 idx)
{
  bmnode_assert (b);
  assert (idx <= *b->nkeys);

  // Move right half up 2 (1 for the expansion and 1 for new idx spot)
  if (idx < *b->nkeys)
    {
      size_t tomove = *b->nkeys - idx;
      memmove (&b->offsets[idx + 2], &b->offsets[idx], tomove * sizeof *b->offsets);
    }

  // Move left half up 1
  if (idx > 0)
    {
      memmove (&b->offsets[1], b->offsets, idx * sizeof *b->offsets);
    }

  (*b->nkeys)++;
  bmnode_init (b);
}

static inline void
unsafe_bmnode_expand_one (bmnode *b)
{
  bmnode_assert (b);
  memmove (&b->offsets[1], b->offsets, *b->nkeys * sizeof *b->offsets);
  (*b->nkeys)++;
  bmnode_init (b);
}

/////////////////////////////// MAIN METHODS
bmnode
bmnode_create (
    bmnode page,
    char *key,
    keylen_t keylen,
    u64 data)
{
  bmnode ret;
  ret.data = page;
  memset (page, 0, PAGE_SIZE);

  bmnode_init (&ret);
  bmnir stat = bmnode_insert_kv (&ret, key, keylen, data);
  assert (stat == BMNIR_SUCCESS);

  return ret;
}

bmnir
bmnode_insert_kv (
    bmnode *dest,
    const char *key,
    keylen_t keylen,
    u64 data)
{
  bmnode_assert (dest);
  assert (key);
  assert (keylen > 0);

  // Check if the kv already exists
  u32 idx;
  if (bmnode_find_key (dest, key, keylen, &idx))
    {
      return BMNIR_DUPLICATE;
    }
  assert (idx < *dest->nkeys);

  // Check for remaining space
  offset_t prefix_tail = bmnode_prefix_tail (dest);
  offset_t kv_start = bmnode_min_offset (dest);

  assert (kv_start > prefix_tail);

  size_t remaining = (size_t)(kv_start - prefix_tail);
  size_t kv_size = sizeof (keylen_t) + (size_t)keylen + sizeof (u64);
  size_t toadd = sizeof (u64) + sizeof (offset_t) + kv_size;

  if (remaining < toadd)
    {
      return BMNIR_FULL;
    }

  // Expand prefix
  unsafe_bmnode_expand_and_split (dest, idx);
  dest->data[idx] = 0;

  assert (kv_start > kv_size);

  // Insert new key and update offsets
  dest->offsets[idx] = kv_start - kv_size;

  u8 *head = &dest->data[dest->offsets[idx]];

  memcpy (head, &keylen, sizeof keylen);
  head += sizeof keylen;
  memcpy (head, key, keylen);
  head += keylen;
  memcpy (head, &data, sizeof data);

  return BMNIR_SUCCESS;
}

int
bmnode_find_key (
    const bmnode *b,
    const char *key,
    keylen_t keylen,
    u32 *idx)
{
  bmnode_assert (b);
  assert (key);
  assert (keylen > 0);
  assert (idx);

  int left = 0;
  int right = *b->nkeys - 1;

  while (left <= right)
    {
      int mid = left + (right - left) / 2;

      bmnode_kv _k = bmnode_kv_create (&b->page[b->offsets[mid]]);

      keylen_t klen = MIN (*_k.keylen, keylen);
      int cmp = strncmp (_k.key, key, klen);

      if (cmp == 0)
        {
          if (*_k.keylen == keylen)
            {
              *idx = mid;
              return 1;
            }
          else if (keylen < *_k.keylen)
            {
              right = mid - 1;
            }
          else
            {
              left = mid + 1;
            }
        }
      else if (cmp < 0)
        {
          left = mid + 1;
        }
      else
        {
          right = mid - 1;
        }
    }

  *idx = left;
  return 0;
}

bmnode_kv
bmnode_get_kv (const bmnode *b, u32 idx)
{
  bmnode_assert (b);
  assert (idx < *b->nkeys);
  bmnode_kv ret = bmnode_kv_create (&b->page[b->offsets[idx]]);
  bmnode_kv_assert (&ret);
  return ret;
}

// Some testing utils
#define BNODE_KV_FROM(cstr, _ptr) \
  ((bnode_kv){ .keylen = strlen (cstr), .key = (char *)(cstr), .ptr = (_ptr) })

#define test_assert_bnode_kv_equal(_k0, _k1)                              \
  do                                                                      \
    {                                                                     \
      test_assert_equal ((_k0).keylen, (_k1).keylen, "%" PRIu16);         \
      test_assert_equal (strncmp ((_k0).key, (_k1).key, (_k0).keylen), 0, \
                         "%" PRId32);                                     \
      test_assert_equal ((_k0).ptr, (_k1).ptr, "%" PRIu64);               \
    }                                                                     \
  while (0)

TEST (insertion)
{
  u8 page[PAGE_SIZE];
  bmnode b = bmnode_create (page, "foobar", 6, 1);
  test_assert_int_equal (bmnode_insert_kv (&b, "fizb", 4, 2), BMNIR_SUCCESS);
  test_assert_int_equal (bmnode_insert_kv (&b, "bazbi", 5, 3), BMNIR_SUCCESS);
  test_assert_int_equal (bmnode_insert_kv (&b, "barbuz", 6, 4), BMNIR_SUCCESS);
  test_assert_int_equal (bmnode_insert_kv (&b, "bar", 3, 5), BMNIR_SUCCESS);
  test_assert_int_equal (bmnode_insert_kv (&b, "barbuzbiz", 9, 6), BMNIR_SUCCESS);
  test_assert_int_equal (bmnode_insert_kv (&b, "bazbi", 5, 9), BMNIR_DUPLICATE);

  u32 idx;
  test_assert_int_equal (bmnode_find_key (&b, "bar", 3, &idx), 5);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 3, 1);
  test_assert_int_equal (strncmp (kv.key, "bar", 3));
  test_assert_int_equal (*(int *)kv.data, 5);

  test_assert_int_equal (bmnode_find_key (&b, "barbuz", 6, &idx), 4);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 6, 1);
  test_assert_int_equal (strncmp (kv.key, "barbuz", 6));

  test_assert_int_equal (bmnode_find_key (&b, "barbuzbiz", 9, &idx), 6);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 9, 1);
  test_assert_int_equal (strncmp (kv.key, "barbuzbiz", 9));

  test_assert_int_equal (bmnode_find_key (&b, "bazbi", 5, &idx), 9);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 5, 1);
  test_assert_int_equal (strncmp (kv.key, "bazbi", 5));

  test_assert_int_equal (bmnode_find_key (&b, "fizb", 4, &idx), 2);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 4, 1);
  test_assert_int_equal (strncmp (kv.key, "fizb", 4));

  test_assert_int_equal (bmnode_find_key (&b, "foobar", 6, &idx), 1);
  bmnode_kv kv = bmnode_get_kv (&b, idx);
  test_assert_int_equal (*kv.keylen, 6, 1);
  test_assert_int_equal (strncmp (kv.key, "foobar", 6));
}

TEST (find)
{
}
