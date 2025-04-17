#include "cbuffer.h"
#include "core/numbers.h"
#include "os/stdlib.h"

cbuffer
cbuffer_create (u8 *data, u32 cap)
{
  ASSERT (data);
  ASSERT (cap > 0);
  return (cbuffer){
    .head = 0,
    .tail = 0,
    .cap = cap,
    .data = data,
    .isfull = 0,
  };
}

u32
cbuffer_len (const cbuffer *b)
{
  int len;
  if (b->isfull)
    {
      len = b->cap;
    }
  else if (b->head >= b->tail)
    {
      len = b->head - b->tail;
    }
  else
    {
      len = b->cap - (b->tail - b->head);
    }
  return len;
}

u32
cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b)
{
  cbuffer_assert (b);
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n > 0);

  u32 len = cbuffer_len (b) / size;
  u32 ntoread = (n < len) ? n : len;
  u32 btoread = ntoread * size;
  u32 bread = 0;

  u8 *output = (u8 *)dest;

  while (bread < btoread)
    {
      u32 next;

      if (!b->isfull && b->head > b->tail)
        {
          next = MIN (b->head - b->tail, btoread - bread);
        }
      else
        {
          next = MIN (b->cap - b->tail, btoread - bread);
        }

      i_memcpy (output + bread, b->data + b->tail, next);
      b->tail = (b->tail + next) % b->cap;
      bread += next;
      b->isfull = 0;
    }

  ASSERT (ntoread == bread * size);
  return ntoread;
}

u32
cbuffer_write (const void *src, u32 size, u32 n, cbuffer *b)
{
  cbuffer_assert (b);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);

  ASSERT (b->cap > cbuffer_len (b));
  u32 avail = (b->cap - cbuffer_len (b)) / size;
  u32 ntowrite = (n < avail) ? n : avail;
  u32 btowrite = ntowrite * size;
  u32 bwrite = 0;

  u8 *input = (u8 *)src;

  while (bwrite < btowrite)
    {
      u32 next;

      ASSERT (btowrite > bwrite);
      if (b->head >= b->tail)
        {
          ASSERT (b->cap > b->head);
          next = MIN (b->cap - b->head, btowrite - bwrite);

          if (next == 0)
            {
              b->head = 0;
              next = b->tail;
            }
        }
      else
        {
          ASSERT (b->tail > b->head);
          next = MIN (b->tail - b->head, btowrite - bwrite);
        }

      i_memcpy (b->data + b->head, input + bwrite, next);
      b->head = (b->head + next) % b->cap;
      bwrite += next;

      if (b->head == b->tail)
        {
          b->isfull = 1;
        }
    }

  ASSERT (ntowrite == bwrite * size);
  return ntowrite;
}

cbuffer_mgr
cbuffer_mgr_create (cbuffer *bufs, int *is_present, u32 len)
{
  ASSERT (bufs);
  ASSERT (is_present);
  ASSERT (len > 0);

  i_memset (is_present, 0, sizeof *is_present * len);

  return (cbuffer_mgr){
    .bufs = bufs,
    .is_present = is_present,
    .len = len,
    .idx = 0,
  };
}

cbuffer *
bmgr_get (cbuffer_mgr *bufs, int *idx)
{
  ASSERT (idx);
  for (u32 i = 0; i < bufs->len; ++i)
    {
      u32 _i = (bufs->idx + i) % bufs->len;
      if (bufs->is_present[_i] == 0)
        {
          bufs->is_present[_i] = 1;
          *idx = _i;
          return &bufs->bufs[_i];
        }
    }
  return NULL;
}

void
bmgr_release (cbuffer_mgr *bufs, u32 idx)
{
  cbuffer_mgr_assert (bufs);
  ASSERT (idx > 0);
  ASSERT (idx < bufs->len);
  ASSERT (bufs->is_present[idx]);
  ASSERT (cbuffer_len (&bufs->bufs[idx]) == 0); // Clean up after yourself
  bufs->is_present[idx] = 0;
}
