#include "paging/types/data_list.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "paging/page.h"

#define DL_HEDR_OFFSET (0)
#define DL_NEXT_OFFSET (DL_HEDR_OFFSET + sizeof (*((data_list *)0)->header))
#define DL_BLEN_OFFSET (DL_NEXT_OFFSET + sizeof (*((data_list *)0)->next))
#define DL_DATA_OFFSET (DL_BLEN_OFFSET + sizeof (*((data_list *)0)->blen))

DEFINE_DBG_ASSERT_I (data_list, unchecked_data_list, d)
{
  ASSERT (d);
  ASSERT (d->raw);
  ASSERT (d->rlen > 0);
  ASSERT_PTR_IS_IDX (d->raw, d->header, DL_HEDR_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->next, DL_NEXT_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->blen, DL_BLEN_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->data, DL_DATA_OFFSET);
  ASSERT (DL_DATA_OFFSET + 10 < d->rlen); // Let's say at least 10 bytes - arbitrary
}

DEFINE_DBG_ASSERT_I (data_list, valid_data_list, d)
{
  ASSERT (dl_validate (d, NULL) == SUCCESS);
}

err_t
dl_validate (const data_list *d, error *e)
{
  unchecked_data_list_assert (d);

  /**
   * Check valid header
   */
  if (*d->header != (u8)PG_DATA_LIST)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Data List: expected header: %" PRpgh " but got: %" PRpgh,
          (pgh)PG_DATA_LIST, *d->header);
    }

  /**
   * Check that blen is less than data avail
   */
  if (*d->blen > dl_data_size (d->rlen))
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Data List: blen (%" PRp_size ") is more than"
          "maximum blen (%" PRp_size ") for page size: %" PRp_size,
          *d->blen, dl_data_size (d->rlen), d->rlen);
    }

  return SUCCESS;
}

data_list
dl_set_ptrs (u8 *raw, p_size len)
{
  ASSERT (raw);
  ASSERT (len > 0);

  data_list ret;

  // Set pointers
  ret = (data_list){
    .raw = raw,
    .rlen = len,
    .header = (pgh *)&raw[DL_HEDR_OFFSET],
    .next = (pgno *)&raw[DL_NEXT_OFFSET],
    .blen = (p_size *)&raw[DL_BLEN_OFFSET],
    .data = (u8 *)&raw[DL_DATA_OFFSET],
  };

  unchecked_data_list_assert (&ret);

  return ret;
}

p_size
dl_data_size (p_size page_size)
{
  ASSERT (page_size > DL_DATA_OFFSET);
  return page_size - DL_DATA_OFFSET;
}

p_size
dl_avail (const data_list *d)
{
  valid_data_list_assert (d);
  p_size total_avail = dl_data_size (d->rlen);
  ASSERT (total_avail >= *d->blen); // because is_valid
  return total_avail - *d->blen;
}

void
dl_init_empty (data_list *d)
{
  unchecked_data_list_assert (d);
  *d->header = PG_DATA_LIST;
  *d->next = 0;
  *d->blen = 0;
  valid_data_list_assert (d);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

p_size
dl_write (data_list *d, const u8 *src, p_size bytes)
{
  valid_data_list_assert (d);
  ASSERT (src);
  ASSERT (bytes > 0);

  p_size dlen = *d->blen;
  u8 *base = d->raw;
  u8 *tail = d->data + dlen;
  p_size psize = d->rlen;

  ASSERT (tail > base); // is_valid
  p_size used = tail - base;

  ASSERT (psize > used); // is_valid
  p_size avail = psize - used;

  p_size next = MIN (avail, bytes);

  if (next > 0)
    {
      i_memcpy (tail, src, next);
      p_size blen = *d->blen;
      blen = blen + next;
      *d->blen = blen;
    }

  return next;
}

p_size
dl_read (const data_list *d, u8 *dest, p_size offset, p_size bytes)
{
  valid_data_list_assert (d);
  ASSERT (bytes > 0);

  p_size dlen = *d->blen;
  u8 *base = d->data;

  ASSERT (offset <= dlen);

  if (offset == dlen)
    {
      return 0;
    }

  p_size avail = dlen - offset;
  p_size toread = MIN (avail, bytes);

  if (toread > 0 && dest)
    {
      i_memcpy (dest, base + offset, toread);
    }

  return toread;
}

p_size
dl_read_out_from (data_list *d, u8 *dest, p_size offset)
{
  valid_data_list_assert (d);
  ASSERT (dest);

  p_size dlen = *d->blen;
  u8 *head = d->data;

  ASSERT (offset <= dlen);

  if (offset == dlen)
    {
      return 0;
    }

  head += offset;
  dlen -= offset;

  if (dlen > 0)
    {
      i_memcpy (dest, head, dlen);
      *d->blen -= dlen;
    }

  return dlen;
}

pgno
dl_get_next (const data_list *d)
{
  valid_data_list_assert (d);
  return *d->next;
}

void
dl_set_next (data_list *d, pgno next)
{
  valid_data_list_assert (d);
  *d->next = next;
}

p_size
dl_used (const data_list *d)
{
  return *d->blen;
}
