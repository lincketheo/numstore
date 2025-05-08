#include "paging/types/hash_leaf.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging/page.h"
#include "paging/types/inner_node.h"
#include "utils/hashing.h"

#define HL_HEDR_OFFSET (0)
#define HL_NEXT_OFFSET (HL_HEDR_OFFSET + sizeof (*((hash_leaf *)0)->header))
#define HL_DATA_OFFSET (HL_NEXT_OFFSET + sizeof (*((hash_leaf *)0)->next))

DEFINE_DBG_ASSERT_I (hash_leaf, unchecked_hash_leaf, d)
{
  ASSERT (d);
  ASSERT (d->raw);
  ASSERT (d->rlen > 0);
  ASSERT_PTR_IS_IDX (d->raw, d->header, HL_HEDR_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->next, HL_NEXT_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->data, HL_DATA_OFFSET);
  ASSERT (HL_DATA_OFFSET + 10 < d->rlen); // Let's say at least 10 bytes - arbitrary
}

DEFINE_DBG_ASSERT_I (hash_leaf, valid_hash_leaf, d)
{
  ASSERT (hl_is_valid (d));
}

bool
hl_is_valid (const hash_leaf *d)
{
  unchecked_hash_leaf_assert (d);

  /**
   * Check valid header
   */
  if (*d->header != (u8)PG_HASH_LEAF)
    {
      return false;
    }

  return true;
}

p_size
hl_data_len (p_size page_size)
{
  ASSERT (page_size > HL_DATA_OFFSET);
  return page_size - HL_DATA_OFFSET;
}

void
hl_init_empty (hash_leaf *hl)
{
  unchecked_hash_leaf_assert (hl);
  u8 eof_header[HLRC_PFX_LEN];
  i_memset (hl->data, 0, hl_data_len (hl->rlen));
  i_memset (eof_header, 0xFF, sizeof eof_header);

  ASSERT (hl_data_len (hl->rlen) > sizeof (eof_header));
  i_memcpy (hl->data, eof_header, sizeof eof_header);
}

hash_leaf
hl_set_ptrs (u8 *raw, p_size len)
{
  ASSERT (raw);
  ASSERT (len > 0);

  hash_leaf ret;

  // Set pointers
  ret = (hash_leaf){
    .raw = raw,
    .rlen = len,

    .header = (pgh *)&raw[HL_HEDR_OFFSET],
    .next = (pgno *)&raw[HL_NEXT_OFFSET],
    .data = (u8 *)&raw[HL_DATA_OFFSET],
  };

  unchecked_hash_leaf_assert (&ret);

  return ret;
}

pgno
hl_get_next (const hash_leaf *hl)
{
  valid_hash_leaf_assert (hl);
  return *hl->next;
}
