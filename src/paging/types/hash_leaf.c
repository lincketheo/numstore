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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

err_t
hlrc_create (
    hlr_cursor *hc,
    const hash_leaf *hl,
    hlrc_params params)
{
  hlr_cursor _hc = (hlr_cursor){
    .state = HLRCS_READING_HEADER,

    .pos = 0,

    .hidx = 0,

    .vstr_alloc = params.vstr_alloc,
    .tstr_alloc = params.tstr_alloc,
  };

  err_t ret = hlrc_scroll (&_hc, hl);
  if (ret)
    {
      return ret;
    }

  *hc = _hc;

  return SUCCESS;
}

hl_header
hlh_parse (u8 header[HLRC_PFX_LEN])
{
  u16 vstrlen = ((u16 *)header)[0];
  u16 tstrlen = ((u16 *)header)[1];
  u8 type = *(u8 *)header + 2 * sizeof (u16);
  pgno pg0 = *(pgno *)header + 2 * sizeof (u16) + sizeof (u8);

  switch (type)
    {
    case (u8)HLRCH_TOMBSTONE:
    case (u8)HLRCH_EOF:
    case (u8)HLRCH_PRESENT:
      {
        break;
      }
    default:
      {
        return (hl_header){ .type = HLRCH_UNKNOWN };
      }
    }

  return (hl_header){
    .vstrlen = vstrlen,
    .tstrlen = tstrlen,
    .pg0 = pg0,
    .type = type,
  };
}

static inline err_t
hlrc_header_read (hlr_cursor *hc)
{
  /**
   * Read header
   */

  // Done reading header - free to set

  switch (type)
    {
    case (u8)HLRCH_TOMBSTONE:
      {
        hc->state = HLRCS_SKIPPING;
        return SUCCESS;
      }
    case (u8)HLRCH_EOF:
      {
        hc->state = HLRCS_EOF;
        return SUCCESS;
      }
    case (u8)HLRCH_PRESENT:
      {
        hc->state = HLRCS_READING_VSTR;
        break;
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }

  char *vstr = lmalloc (hc->vstr_alloc, vstrlen);
  char *tstr = lmalloc (hc->tstr_alloc, tstrlen);

  if (vstr == NULL || tstr == NULL)
    {
      if (vstr)
        {
          lfree (hc->vstr_alloc, vstr);
        }
      if (tstr)
        {
          lfree (hc->tstr_alloc, tstr);
        }
      return ERR_NOMEM;
    }

  hc->type = type;
  hc->pg0 = pg0;

  hc->vstr.data = vstr;
  hc->vstr.len = vstrlen;
  hc->vidx = 0;

  hc->tstr.data = tstr;
  hc->tstr.len = tstrlen;
  hc->tidx = 0;

  return SUCCESS;
}

static inline err_t
hlrc_read_header (hlr_cursor *hc, const hash_leaf *hl)
{
  ASSERT (hc->state == HLRCS_READING_HEADER);
  ASSERT (hc->hidx < HLRC_PFX_LEN);

  p_size avail = hl_data_len (hl->rlen) - hc->pos;
  p_size next = MIN (HLRC_PFX_LEN - hc->hidx, avail);

  ASSERT (next > 0);

  i_memcpy (hc->header + hc->hidx, &hl->data[hc->pos], next);
  hc->pos += next;
  hc->hidx += next;

  /**
   * State transition
   */
  if (hc->hidx == HLRC_PFX_LEN)
    {
      err_t ret = hlrc_header_read (hc);
      if (ret)
        {
          return ret;
        }
    }

  return SUCCESS;
}

static inline void
hlrc_read_vstr (hlr_cursor *hc, const hash_leaf *hl)
{
  ASSERT (hc->state == HLRCS_READING_VSTR);
  ASSERT (hc->vidx < hc->vstr.len);

  u16 vleft = hc->vstr.len - hc->vidx;
  p_size avail = hl_data_len (hl->rlen) - hc->pos;
  p_size next = MIN (vleft, avail);

  ASSERT (next > 0);

  i_memcpy (hc->vstr.data + hc->vidx, &hl->data[hc->pos], next);
  hc->pos += next;
  hc->vidx += next;

  /**
   * State transition
   */
  if (hc->vidx == hc->vstr.len)
    {
      hc->state = HLRCS_READING_TSTR;
    }
}

static inline void
hlrc_read_tstr (hlr_cursor *hc, const hash_leaf *hl)
{
  ASSERT (hc->state == HLRCS_READING_TSTR);
  ASSERT (hc->tidx < hc->tstr.len);

  u16 tleft = hc->vstr.len - hc->vidx;
  p_size avail = hl_data_len (hl->rlen) - hc->pos;
  p_size next = MIN (tleft, avail);

  ASSERT (next > 0);

  i_memcpy (hc->tstr.data + hc->tidx, &hl->data[hc->pos], next);
  hc->pos += next;
  hc->tidx += next;

  /**
   * State transition
   */
  if (hc->tidx == hc->tstr.len)
    {
      hc->state = HLRCS_DONE;
    }
}

err_t
hlrc_scroll (hlr_cursor *hc, const hash_leaf *hl)
{
  if (hc->state == HLRCS_READING_HEADER)
    {
      err_t ret = hlrc_read_header (hc, hl);
      if (ret)
        {
          return ret;
        }
    }

  if (hc->state == HLRCS_READING_VSTR)
    {
      hlrc_read_vstr (hc, hl);
    }

  if (hc->state == HLRCS_READING_TSTR)
    {
      hlrc_read_tstr (hc, hl);
    }

  if (hc->state != HLRCS_EOF && hl_data_len (hl->rlen) == hc->pos)
    {
      hc->wants_new_page = true;
    }
  else
    {
      hc->wants_new_page = false;
    }

  return SUCCESS;
}

hlrc_ret
hlrc_consume (hlr_cursor *hc)
{
  ASSERT (hc->state == HLRCS_DONE);

  hlrc_ret ret = {
    .vstr = hc->vstr,
    .tstr = hc->tstr,
  };

  hc->state = HLRCS_READING_HEADER;

  return ret;
}
