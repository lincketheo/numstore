#include "mm/lalloc.h"
#include "dev/testing.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"

/////////////////////// Limited Allocator

DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
  ASSERT (l->data);
  ASSERT (l->used <= l->limit);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

lalloc
lalloc_create (u8 *data, u32 limit)
{
  ASSERT (limit > 0);
  lalloc ret = {
    .used = 0,
    .limit = limit,
    .data = data,
  };
  lalloc_assert (&ret);
  return ret;
}

lalloc
lalloc_reserve_remaining (lalloc *from)
{
  lalloc_assert (from);
  u32 avail = from->limit - from->used;
  return lalloc_create (from->data + from->used, avail);
}

err_t
lalloc_reserve (lalloc *dest, lalloc *from, u32 amount, error *e)
{
  lalloc_assert (from);
  ASSERT (dest);
  ASSERT (amount > 0);
  u32 avail = from->limit - from->used;
  if (amount > avail)
    {
      return error_causef (e, ERR_NOMEM, "Failed to reserve memory");
    }
  *dest = lalloc_create (from->data + from->used, amount);
  from->used += amount;
  lalloc_assert (dest);
  return SUCCESS;
}

void *
lmalloc (lalloc *a, u32 req, u32 size)
{
  lalloc_assert (a);
  ASSERT (req > 0);
  ASSERT (size > 0);

  u32 total;
  if (!SAFE_MUL_U32 (&total, req, size))
    {
      return NULL;
    }

  u32 avail = a->limit - a->used;
  if (avail <= total)
    {
      return NULL;
    }

  void *ret = &a->data[a->used];
  a->used += total;

  return ret;
}

void *
lcalloc (lalloc *a, u32 req, u32 size)
{
  void *ret = lmalloc (a, req, size);
  if (ret == NULL)
    {
      return ret;
    }

  i_memset (ret, 0, req * size);

  return ret;
}

void
lalloc_reset (lalloc *a)
{
  lalloc_assert (a);
  a->used = 0;
}
