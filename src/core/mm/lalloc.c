#include "core/mm/lalloc.h"
#include "core/dev/testing.h"
#include "core/errors/error.h"
#include "core/intf/logging.h"
#include "core/intf/stdlib.h"
#include "core/utils/bounds.h"

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

u32
lalloc_get_state (const lalloc *l)
{
  return l->used;
}

void
lalloc_reset_to_state (lalloc *l, u32 state)
{
  l->used = state;
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
  a->used = a->used + total;

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

TEST (lalloc_edge_cases)
{
  u8 mem[64];
  lalloc a = lalloc_create (mem, sizeof (mem));

  test_assert_int_equal (lalloc_get_state (&a), 0);
  test_assert_int_equal (a.limit, sizeof (mem));

  // first allocation (1 byte) must succeed and be correctly aligned
  void *p1 = lmalloc (&a, 1, 1);
  test_fail_if_null (p1);
  size_t align = sizeof (void *);
  test_assert_int_equal (((uintptr_t)p1) % align, 0);

  u32 s1 = lalloc_get_state (&a);

  // lcalloc must zero the returned memory
  int *p2 = lcalloc (&a, 4, sizeof (int));
  test_fail_if_null (p2);
  for (int i = 0; i < 4; ++i)
    {
      test_assert_int_equal (p2[i], 0);
    }

  // rewind with lalloc_reset_to_state
  lalloc_reset_to_state (&a, s1);
  test_assert_int_equal (lalloc_get_state (&a), s1);

  // allocate until only one byte is left - should still succeed
  u32 left = a.limit - a.used;
  void *p3 = lmalloc (&a, left - 1, 1);
  test_fail_if_null (p3);

  // allocator now “full” – further request must fail AND keep state
  u32 before_fail = lalloc_get_state (&a);
  void *p_fail = lmalloc (&a, 2, 1);
  test_assert_int_equal (p_fail == NULL, true);
  test_assert_int_equal (lalloc_get_state (&a), before_fail);

  // overflow protection: extremely large request must return NULL
  before_fail = lalloc_get_state (&a);
  void *p_over = lmalloc (&a, UINT32_MAX, 16);
  test_assert_int_equal (p_over == NULL, true);
  test_assert_int_equal (lalloc_get_state (&a), before_fail);

  // lalloc_reset should clear all usage
  lalloc_reset (&a);
  test_assert_int_equal (lalloc_get_state (&a), 0);
}
