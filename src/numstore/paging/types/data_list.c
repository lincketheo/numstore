#include "numstore/paging/types/data_list.h"

#include "core/dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "core/intf/stdlib.h" // i_memcpy

#include "numstore/paging/page.h" // PG_DATA_LIST

DEFINE_DBG_ASSERT_I (data_list, unchecked_data_list, d)
{
  ASSERT (d);
  ASSERT (DL_DATA_OFST + 10 < PAGE_SIZE);
}

DEFINE_DBG_ASSERT_I (data_list, valid_data_list, d)
{
  error e = error_create (NULL);
  ASSERT (dl_validate (d, &e) == SUCCESS);
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
  if (*d->blen > DL_DATA_SIZE)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Data List: blen (%" PRp_size ") is more than"
          "maximum blen (%" PRp_size ") for page size: %" PRp_size,
          *d->blen, DL_DATA_SIZE, PAGE_SIZE);
    }

  return SUCCESS;
}

void
dl_set_ptrs (data_list *dl, u8 raw[PAGE_SIZE])
{
  *dl = (data_list){
    .header = (pgh *)&raw[DL_HEDR_OFST],
    .next = (pgno *)&raw[DL_NEXT_OFST],
    .blen = (p_size *)&raw[DL_BLEN_OFST],
    .data = (u8 *)&raw[DL_DATA_OFST],
  };
  unchecked_data_list_assert (dl);
}

p_size
dl_avail (const data_list *d)
{
  valid_data_list_assert (d);
  p_size total_avail = DL_DATA_SIZE;

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
dl_append (data_list *d, const u8 *src, p_size bytes)
{
  valid_data_list_assert (d);
  ASSERT (src);
  ASSERT (bytes > 0);

  p_size avail = DL_DATA_SIZE - *d->blen;
  p_size next = MIN (avail, bytes);

  if (next > 0)
    {
      u8 *tail = d->data + *d->blen;

      i_memcpy (tail, src, next);

      *d->blen += next;
    }

  return next;
}

p_size
dl_write (data_list *d, const u8 *src, p_size offset, p_size bytes)
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

  if (toread > 0 && src)
    {
      i_memcpy (base + offset, src, toread);
    }

  return toread;
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

void
i_log_dl (const data_list *dl)
{
  valid_data_list_assert (dl);

  i_log_info ("=== DATA LIST PAGE START ===\n");

  i_log_info ("HEADER     : %" PRpgh "\n", *dl->header);
  i_log_info ("NEXT_PAGE  : %" PRpgno "\n", *dl->next);
  i_log_info ("BYTE_LEN   : %" PRp_size "\n", *dl->blen);

  for (u32 i = 0; i < *dl->blen; ++i)
    {
      i_log_info ("  DATA[%u] = 0x%02x\n", i, dl->data[i]);
    }

  i_log_info ("=== DATA LIST PAGE END ===\n");
}
