#include "paging/page.h"
#include "dev/assert.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"

///////////////////////////// PAGE TYPES

#define MIN_PAGE_SIZE 512

DEFINE_DBG_ASSERT_I (page, page, p)
{
  ASSERT (p);
  ASSERT (p->raw);
  ASSERT (p->header);
  ASSERT (p->len >= MIN_PAGE_SIZE);
}

static inline void
page_set (page *p, page_interpret_params params)
{
  // All pages have a header in the first 8 bytes
  u32 idx = 0;
  p->raw = params.raw;
  p->pgno = params.pgno;
  p->len = params.page_size;

#define advance(vname, type)        \
  do                                \
    {                               \
      vname = (type *)&p->raw[idx]; \
      idx += sizeof *vname;         \
    }                               \
  while (0)

  advance (p->header, u8);

  switch (params.type)
    {
    case PG_DATA_LIST:
      {
        advance (p->dl.next, i64);
        advance (p->dl.len_num, u16);
        advance (p->dl.len_denom, u16);
        advance (p->dl.data, u8);
        break;
      }
    case PG_INNER_NODE:
      {
        advance (p->in.nkeys, u16);
        advance (p->in.leafs, u64);
        ASSERT (can_mul_u16 (sizeof (u64), *p->in.nkeys));
        ASSERT (can_sub_u32 (p->len, sizeof (u64) * *p->in.keys));
        p->in.keys = (u64 *)&p->raw[p->len - (sizeof (u64) * *p->in.nkeys)];
        break;
      }
    case PG_HASH_PAGE:
      {
        advance (p->hp.len, u32);
        advance (p->hp.hashes, u64);
        break;
      }
    case PG_HASH_LEAF:
      {
        advance (p->hl.next, u64);
        advance (p->hl.nvalues, u16);
        advance (p->hl.offsets, u16);
        break;
      }
    }

#undef advance
}

err_t
page_read_expect (page *dest, page_interpret_params params)
{
  ASSERT (dest);

  page_set (dest, params);

  if (!(*dest->header & params.type))
    {
      return ERR_INVALID_STATE;
    }

  return SUCCESS;
}

void
page_init (page *dest, page_interpret_params params)
{
  ASSERT (dest);
  ASSERT (params.raw);

  i_memset (params.raw, 0, params.page_size);

  page_set (dest, params);
  *dest->header = (u8)params.type;

  // Aditional Setup
  switch (params.type)
    {
    case PG_DATA_LIST:
      {
        break;
      }
    case PG_INNER_NODE:
      {
        break;
      }
    case PG_HASH_PAGE:
      {
        ASSERT (params.page_size > sizeof (u32));
        *dest->hp.len = (params.page_size - sizeof (u32)) / sizeof (i64);
        break;
      }
    case PG_HASH_LEAF:
      {
        break;
      }
    }
}
