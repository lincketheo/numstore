#include "paging/types/hash_leaf.h"
#include "dev/assert.h"
#include "dev/testing.h"
#include "intf/stdlib.h"
#include "paging/page.h"

/**
err_t
hl_get_tuple (hash_leaf_tuple *dest, const page *p, p_size idx)
{
  ASSERT (dest);
  ASSERT (*p->header == PG_HASH_LEAF);

  p_size nvals = *p->hl.nvalues;
  ASSERT (idx < nvals);

  u8 *base = p->raw;
  u32 off = p->hl.offsets[idx];

#define BOUND_AND_ADVANCE(_len)  \
  do                             \
    {                            \
      if (off + (_len) > p->len) \
        {                        \
          goto overflow;         \
        }                        \
      off += (_len);             \
    }                            \
  while (0)

  // strlen
  BOUND_AND_ADVANCE (sizeof *dest->strlen);
  dest->strlen = (p_size *)(base + off - sizeof *dest->strlen);
  p_size slen = *dest->strlen;

  // str
  BOUND_AND_ADVANCE (slen);
  dest->str = (char *)(base + off - slen);

  // pg0
  BOUND_AND_ADVANCE (sizeof *dest->pg0);
  dest->pg0 = (u64 *)(base + off - sizeof *dest->pg0);

  // tstrlen
  BOUND_AND_ADVANCE (sizeof *dest->tstrlen);
  dest->tstrlen = (p_size *)(base + off - sizeof *dest->tstrlen);
  p_size tlen = *dest->tstrlen;

  // tstr
  BOUND_AND_ADVANCE (tlen);
  dest->tstr = (char *)(base + off - tlen);

#undef BOUND_AND_ADVANCE

  return SUCCESS;

overflow:
  return ERR_INVALID_STATE;
}

TEST (hl_get_tuple)
{
  // Green path
  u8 raw1[64] = { 0 };

  page p1 = {
    .raw = raw1,
    .len = 64,
    .header = raw1,
    .pgno = 42,
    .hl = {
        .nvalues = &(p_size){ 1 },
        .offsets = (p_size[]){ 8 },
    }
  };
  raw1[0] = PG_HASH_LEAF;

  // Build tuple
  p_size slen = 3;
  i_memcpy (raw1 + 8, &slen, sizeof slen);
  i_memcpy (raw1 + 10, "bar", slen);

  u64 pg0 = 0x12345678;
  i_memcpy (raw1 + 13, &pg0, sizeof pg0);

  p_size tlen = 4;
  i_memcpy (raw1 + 21, &tlen, sizeof tlen);
  i_memcpy (raw1 + 23, "test", tlen);

  hash_leaf_tuple dest;
  err_t r1 = hl_get_tuple (&dest, &p1, 0);

  test_assert_int_equal ((int)r1, (int)SUCCESS);
  test_assert_int_equal ((int)*dest.strlen, (int)slen);
  test_assert_int_equal ((int)*dest.pg0, (int)pg0);
  test_assert_int_equal ((int)*dest.tstrlen, (int)tlen);

  // Red path - overflow
  u8 raw2[16] = { 0 };

  page p2 = {
    .raw = raw2,
    .len = 16,
    .header = raw2,
    .pgno = 99,
    .hl = {
        .nvalues = &(p_size){ 1 },
        .offsets = (p_size[]){ 12 },
    }
  };
  raw2[0] = PG_HASH_LEAF;

  err_t r2 = hl_get_tuple (&dest, &p2, 0);
  test_assert_int_equal ((int)r2, (int)ERR_INVALID_STATE);
}
*/
