#include "dev/testing.h"
#include "intf/mm.h"
#include "utils/bounds.h"
#include <errno.h>

/////////////////////// Allocation
DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
}

#define ALLOC_HEADER sizeof (u64)

static inline u64 *
alloc_header_ptr (void *data)
{
  return (u64 *)data - 1;
}

static inline u64
alloc_header_get (void *data)
{
  return *alloc_header_ptr (data);
}

static inline void
alloc_header_set (void *data, u64 bytes)
{
  *alloc_header_ptr (data) = bytes;
}

lalloc
lalloc_create (u64 limit)
{
  return (lalloc){
    .limit = limit,
    .total = 0,
  };
}

void *
lmalloc (lalloc *a, u64 bytes)
{
  lalloc_assert (a);
  ASSERT (bytes > 0);

  // check payload + header fits
  ASSERT (can_add_u64 (bytes, ALLOC_HEADER));
  bytes += ALLOC_HEADER;
  ASSERT (can_add_u64 (a->total, bytes));

  u64 new_total = a->total + bytes;
  if (a->limit > 0 && new_total > a->limit)
    {
      return NULL;
    }

  // system allocation
  u64 *p = malloc ((size_t)bytes);
  if (p == NULL)
    {
      return NULL;
    }

  // store header and return user pointer
  alloc_header_set (&p[1], bytes);
  a->total = new_total;
  return &p[1];
}

void *
lrealloc (lalloc *a, void *data, u64 bytes)
{
  lalloc_assert (a);
  ASSERT (data);
  ASSERT (bytes > 0);

  u64 *oldp = alloc_header_ptr (data);
  u64 old_size = alloc_header_get (data);

  ASSERT (can_add_u64 (bytes, ALLOC_HEADER));
  ASSERT (can_sub_u64 (a->total, old_size));
  ASSERT (can_add_u64 (a->total - old_size, bytes + ALLOC_HEADER));

  u64 new_total = a->total - old_size + (bytes + ALLOC_HEADER);
  if (a->limit > 0 && new_total > a->limit)
    {
      return NULL;
    }

  u64 *p = realloc (oldp, (size_t)(bytes + ALLOC_HEADER));
  if (p == NULL)
    {
      return NULL;
    }

  alloc_header_set (&p[1], bytes + ALLOC_HEADER);
  a->total = new_total;
  return &p[1];
}

void
lfree (lalloc *a, void *data)
{
  ASSERT (data != NULL);
  u64 size = alloc_header_get (data);
  ASSERT (can_sub_u64 (a->total, size));
  a->total -= size;
  free (alloc_header_ptr (data));
}

TEST (lalloc)
{
  // limited allocator
  lalloc a = lalloc_create (100);
  u8 *pt1 = lmalloc (&a, 10);
  test_assert_int_equal ((int)a.total, ALLOC_HEADER + 10);
  test_assert_int_equal (pt1 != NULL, 1);

  u8 *pt2 = lmalloc (&a, 2 * 5);
  test_assert_int_equal (pt2 != NULL, 1);
  test_assert_int_equal ((int)a.total, ALLOC_HEADER + 10 + ALLOC_HEADER + 2 * 5);

  pt1 = lrealloc (&a, pt1, 5);
  test_assert_int_equal (pt1 != NULL, 1);
  test_assert_int_equal ((int)a.total, ALLOC_HEADER + 5 + ALLOC_HEADER + 2 * 5);

  lfree (&a, pt2);
  test_assert_int_equal ((int)a.total, ALLOC_HEADER + 5);

  lfree (&a, pt1);
  test_assert_int_equal ((int)a.total, 0);

  // unlimited allocator
  lalloc b = lalloc_create (0);

  pt1 = lmalloc (&b, 2 * 5);
  test_assert_int_equal (pt1 != NULL, 1);
  test_assert_int_equal ((int)b.total, ALLOC_HEADER + 2 * 5);

  pt1 = lrealloc (&b, pt1, 5);
  test_assert_int_equal (pt1 != NULL, 1);
  test_assert_int_equal ((int)b.total, ALLOC_HEADER + 5);

  lfree (&b, pt1);
  test_assert_int_equal ((int)b.total, 0);
}
