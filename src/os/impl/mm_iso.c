#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"
#include <errno.h>

/////////////////////// Allocation

DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
  if (l->data)
    {
      ASSERT (l->cdlen <= l->cdcap);
      ASSERT (l->cdcap > 0);
    }
  else
    {
      ASSERT (l->cdlen == 0);
      ASSERT (l->cdcap == 0);
    }

  ASSERT (l->dyn_used <= l->dyn_limit);
}

err_t
lalloc_create (lalloc *dest, u64 climit, u32 dlimit)
{
  ASSERT (dest);

  u8 *data = NULL;

  if (dlimit > 0)
    {
      data = malloc (climit);
      if (data == NULL)
        {
          return ERR_NOMEM;
        }
    }

  *dest = (lalloc){
    .data = data,
    .cdlen = 0,
    .cdcap = climit,
    .dyn_used = 0,
    .dyn_limit = dlimit,
  };
  lalloc_assert (dest);

  return SUCCESS;
}

void
lalloc_create_from (lalloc *dest, u8 *data, u64 climit, u32 dlimit)
{
  ASSERT (dest);

  *dest = (lalloc){
    .data = data,
    .cdlen = 0,
    .cdcap = climit,
    .dyn_used = 0,
    .dyn_limit = dlimit,
  };

  lalloc_assert (dest);
}

void
lalloc_free (lalloc *l)
{
  lalloc_assert (l);

  if (l->cdcap > 0)
    {
      free (l->data);
      l->data = NULL;
      l->cdcap = 0;
      l->cdlen = 0;
    }

  ASSERT (l->dyn_used == 0);
  l->dyn_used = 0;
  l->dyn_limit = 0;

  lalloc_assert (l);
}

void *
lmalloc_dyn (lalloc *a, u32 bytes)
{
  lalloc_assert (a);
  ASSERT (bytes > 0);

  // check payload + header fits
  if (!can_add_u32 (bytes, sizeof (u32)))
    {
      return NULL;
    }
  bytes += sizeof (u32);
  if (!can_add_u32 (a->dyn_used, bytes))
    {
      return NULL;
    }

  // No space available
  u32 new_dyn_used = a->dyn_used + bytes;
  if (a->dyn_limit > 0 && new_dyn_used > a->dyn_limit)
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
  a->dyn_used = new_dyn_used;

  return &p[1];
}

void *
lmalloc_const (lalloc *a, u64 bytes)
{
  lalloc_assert (a);
  ASSERT (bytes > 0);

  if (!can_add_u64 (a->cdlen, bytes))
    {
      return NULL;
    }
  if (a->cdlen + bytes > a->cdcap)
    {
      return NULL;
    }
  u64 idx = a->cdlen;
  a->cdlen += bytes;
  return &a->data[idx];
}

void *
lcalloc_dyn (lalloc *a, u32 n, u32 size)
{
  lalloc_assert (a);
  ASSERT (n > 0);
  ASSERT (size > 0);

  if (!can_mul_u32 (n, size))
    {
      return NULL;
    }

  void *ret = lmalloc_dyn (a, n * size);
  if (ret != NULL)
    {
      i_memset (ret, 0, n * size);
    }

  return ret;
}

void *
lcalloc_const (lalloc *a, u64 n, u64 size)
{
  lalloc_assert (a);
  ASSERT (n > 0);
  ASSERT (size > 0);

  if (!can_mul_u64 (n, size))
    {
      return NULL;
    }

  void *ret = lmalloc_const (a, n * size);
  if (ret != NULL)
    {
      i_memset (ret, 0, n * size);
    }

  return ret;
}

void *
lrealloc_dyn (lalloc *a, void *data, u32 bytes)
{
  lalloc_assert (a);
  ASSERT (data);
  ASSERT (bytes > 0);

  u32 old_bytes = *((u32 *)data - 1);
  ASSERT (old_bytes > sizeof (u32));
  ASSERT (a->dyn_limit == 0 || old_bytes <= a->dyn_limit);
  ASSERT (old_bytes <= a->dyn_used);
  ASSERT (old_bytes > 0);

  if (!can_add_u32 (bytes, sizeof (u32)))
    {
      return NULL;
    }
  bytes += sizeof (u32);
  ASSERT (can_sub_u32 (a->dyn_used, old_bytes)); // See prev assertion
  if (!can_add_u32 (a->dyn_used - old_bytes, bytes))
    {
      return NULL;
    }
  if (a->dyn_used - old_bytes + bytes > a->dyn_limit)
    {
      return NULL;
    }

  u32 *new_data = realloc ((u8 *)data - 1, bytes);
  if (new_data == NULL)
    {
      return NULL;
    }
  new_data[0] = bytes;

  a->dyn_used = a->dyn_used - old_bytes + bytes;
  lalloc_assert (a);

  return &new_data[1];
}
