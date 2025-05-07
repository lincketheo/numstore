#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"

#include <errno.h>
#include <stdlib.h>

/////////////////////// Limited Allocator

DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
  ASSERT (l->used <= l->limit);
}

void
lalloc_create (lalloc *dest, u32 limit)
{
  ASSERT (dest);

  *dest = (lalloc){
    .used = 0,
    .limit = limit,
  };
  lalloc_assert (dest);
}

void *
lmalloc (lalloc *a, u32 bytes)
{
  lalloc_assert (a);
  ASSERT (bytes > 0);

  // check payload + header fits
  bytes += sizeof (u32);

  // No space available
  u32 new_used = a->used + bytes;
  if (a->limit > 0 && new_used > a->limit)
    {
      return NULL;
    }

  // system allocation
  u32 *p = malloc ((size_t)bytes);
  if (p == NULL)
    {
      return NULL;
    }

  p[0] = bytes;
  a->used = new_used;

  return &p[1];
}

void *
lcalloc (lalloc *a, u32 n, u32 size)
{
  lalloc_assert (a);
  ASSERT (n > 0);
  ASSERT (size > 0);

  void *ret = lmalloc (a, n * size);
  if (ret != NULL)
    {
      i_memset (ret, 0, n * size);
    }

  return ret;
}

void *
lrealloc (lalloc *a, void *data, u32 bytes)
{
  lalloc_assert (a);
  ASSERT (data);
  ASSERT (bytes > 0);

  u32 old_bytes = *((u32 *)data - 1);
  ASSERT (old_bytes > sizeof (u32));
  ASSERT (a->limit == 0 || old_bytes <= a->limit);
  ASSERT (old_bytes <= a->used);
  ASSERT (old_bytes > 0);

  bytes += sizeof (u32);

  if (a->used - old_bytes + bytes > a->limit)
    {
      return NULL;
    }

  u32 *new_data = realloc ((u32 *)data - 1, bytes);
  if (new_data == NULL)
    {
      return NULL;
    }
  new_data[0] = bytes;

  a->used = a->used - old_bytes + bytes;
  lalloc_assert (a);

  return &new_data[1];
}

void *
lalloc_xfer (lalloc *to, lalloc *from, void *data)
{
  lalloc_assert (to);
  lalloc_assert (from);
  ASSERT (data);

  ASSERT (data);

  /**
   * Extract meta information
   */
  u32 *ptr = (u32 *)data;
  u32 bytes = *(ptr - 1);
  ASSERT (bytes > 0);
  ASSERT (from->used >= bytes);

  /**
   * Block on [to] having space available
   */
  if (to->used + bytes > to->limit)
    {
      return NULL;
    }

  /**
   * Subtract used amount from from
   */
  from->used -= bytes;

  /**
   * Add it to [to]
   */
  to->used += bytes;

  return data;
}

void
lfree (lalloc *a, void *data)
{
  lalloc_assert (a);
  ASSERT (data);
  u32 *ptr = (u32 *)data;
  u32 bytes = *(ptr - 1);
  ASSERT (bytes > 0);
  ASSERT (a->used >= bytes);
  free (ptr - 1);
  a->used -= bytes;
}

void
lalloc_release (lalloc *l)
{
  lalloc_assert (l);
  ASSERT (l->used = 0);
  l->used = 0;
  l->limit = 0;
  lalloc_assert (l);
}

//////////////////////////// Scoped Allocator

DEFINE_DBG_ASSERT_I (salloc, salloc, l)
{
  ASSERT (l);
  if (l->data)
    {
      ASSERT (l->len <= l->cap);
      ASSERT (l->cap > 0);
    }
  else
    {
      ASSERT (l->len == 0);
      ASSERT (l->cap == 0);
    }
}

err_t
salloc_create (salloc *dest, u64 cap)
{
  ASSERT (dest);

  u8 *data = NULL;

  if (cap > 0)
    {
      data = malloc (cap);
      if (data == NULL)
        {
          return ERR_NOMEM;
        }
    }

  *dest = (salloc){
    .data = data,
    .len = 0,
    .cap = cap,
  };
  salloc_assert (dest);

  return SUCCESS;
}

void
salloc_create_from (salloc *dest, u8 *data, u64 cap)
{
  ASSERT (dest);

  *dest = (salloc){
    .data = data,
    .len = 0,
    .cap = cap,
  };

  salloc_assert (dest);
}

void *
smalloc (salloc *a, u64 bytes)
{
  salloc_assert (a);
  ASSERT (bytes > 0);

  u64 total = bytes + sizeof (u64);

  if (a->len + total > a->cap)
    {
      return NULL;
    }

  u64 idx = a->len;
  a->len += total;

  // [0, 1, 2, 3, ..., bytes - 1, bytes, bytes + 1, ..., bytes + 7]
  a->data[idx + bytes] = bytes;

  return &a->data[idx];
}

void *
scalloc (salloc *a, u64 n, u64 size)
{
  salloc_assert (a);
  ASSERT (n > 0);
  ASSERT (size > 0);

  void *ret = smalloc (a, n * size);
  if (ret != NULL)
    {
      i_memset (ret, 0, n * size);
    }

  return ret;
}

void
spop (salloc *a)
{
  salloc_assert (a);
  ASSERT (a->len > sizeof (u64));

  a->len -= sizeof (u64);
  u64 bytes = *((u64 *)&a->data[a->len]);
  ASSERT (bytes > 0);

  a->len -= bytes;

  salloc_assert (a);
}
