#include "paging/types/data_list.h"
#include "dev/assert.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "paging/page.h"

DEFINE_DBG_ASSERT_I (data_list, data_list, d)
{
  ASSERT (d);
  ASSERT (d->header);
  ASSERT (d->next);
  ASSERT (d->blen);
  ASSERT (d->data);

  // Minimum page size
  ASSERT (d->raw < d->data);
  ASSERT ((d->data - d->raw) < d->rlen);
}

bool
dl_is_valid (data_list *d)
{
  ASSERT (d);

  if (*d->header != (u8)PG_DATA_LIST)
    {
      return false;
    }

  p_size used = d->data - d->raw;
  p_size avail = d->rlen - used;
  if (*d->blen > avail)
    {
      return false;
    }

  return true;
}

data_list
dl_set_ptrs (u8 *raw, p_size len)
{
  ASSERT (raw);
  ASSERT (len > 0);

  data_list ret;
  p_size head = 0;

  p_size header = head;
  head += sizeof (*ret.header);

  p_size next = head;
  head += sizeof (*ret.next);

  p_size blen = head;
  head += sizeof (*ret.blen);

  p_size data = head;

  // Ensure enough space
  ASSERT (head + 10 <= len);

  // Set pointers
  ret = (data_list){
    .raw = raw,
    .rlen = len,
    .header = (pgh *)&raw[header],
    .next = (pgno *)&raw[next],
    .blen = (p_size *)&raw[blen],
    .data = (u8 *)&raw[data],
  };

  data_list_assert (&ret);

  return ret;
}

p_size
dl_avail (data_list *d)
{
  data_list_assert (d);
  ASSERT (dl_is_valid (d));
  return d->rlen - (d->data + *d->blen - d->raw);
}

void
dl_init_empty (data_list *d)
{
  data_list_assert (d);
  *d->header = PG_DATA_LIST;
  *d->next = 0;
  *d->blen = 0;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

p_size
dl_write (data_list *d, const u8 *src, p_size bytes)
{
  data_list_assert (d);
  ASSERT (dl_is_valid (d));
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
      *d->blen += next;
    }

  return next;
}

p_size
dl_read (data_list *d, u8 *dest, p_size offset, p_size bytes)
{
  data_list_assert (d);
  ASSERT (dl_is_valid (d));
  ASSERT (bytes > 0);

  p_size dlen = *d->blen;
  u8 *base = d->data;

  if (offset >= dlen)
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
  data_list_assert (d);
  ASSERT (dl_is_valid (d));
  ASSERT (dest);

  p_size dlen = *d->blen;
  u8 *head = d->data;

  if (offset >= dlen)
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

void
dl_set_next (data_list *d, pgno next)
{
  data_list_assert (d);
  ASSERT (dl_is_valid (d));
  *d->next = next;
}
