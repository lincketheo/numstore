#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "utils/bounds.h"

#include <errno.h>
#include <malloc.h>
#include <stddef.h> // offsetof
#include <stdlib.h>
#include <string.h>

#define container_of(ptr, type, member) \
  ((type *)(((char *)(ptr)) - offsetof (type, member)))

/////////////////////// Allocation
DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
  ASSERT (l->malloc);
  ASSERT (l->calloc);
  ASSERT (l->realloc);
  ASSERT (l->free);
  ASSERT (l->release);
}

/////////////////////// STDALLOC
DEFINE_DBG_ASSERT_I (stdalloc, stdalloc, s)
{
  ASSERT (s);
  ASSERT (s->limit == 0 || s->total <= s->limit);
  lalloc_assert (&s->alloc);
}

#define ALLOC_HEADER sizeof (u64)

static inline u64 *
alloc_header_ptr (void *data)
{
  return (u64 *)data - 1; // header stored just before user pointer
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

static inline void *
alloc_data_from_header (u64 *hp)
{
  return (void *)(hp + 1);
}

static alloc_ret
stdalloc_malloc (lalloc *iface, u64 bytes)
{
  lalloc_assert (iface);
  ASSERT (bytes > 0);
  stdalloc *sa = container_of (iface, stdalloc, alloc);
  stdalloc_assert (sa);

  // check payload + header fits
  if (!can_add_u64 (bytes, ALLOC_HEADER))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  bytes += ALLOC_HEADER;

  // check total + bytes fits and respects limit
  if (!can_add_u64 (sa->total, bytes))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  u64 new_total = sa->total + bytes;
  if (sa->limit > 0 && new_total > sa->limit)
    {
      return (alloc_ret){ .ret = ERR_OVERFLOW };
    }

  // system allocation
  u64 *p = malloc ((size_t)bytes);
  if (p == NULL)
    {
      if (errno == ENOMEM)
        {
          return (alloc_ret){ .ret = ERR_OVERFLOW };
        }
      else
        {
          i_log_error ("malloc failed for %" PRIu64 " bytes: %s", bytes, strerror (errno));
          return (alloc_ret){ .ret = ERR_IO };
        }
    }

  // store header and return user pointer
  alloc_header_set (&p[1], bytes);
  sa->total = new_total;
  return (alloc_ret){ .ret = SUCCESS, .data = &p[1] };
}

static alloc_ret
stdalloc_calloc (lalloc *iface, u64 nmemb, u64 size)
{
  lalloc_assert (iface);
  ASSERT (nmemb > 0 && size > 0);
  stdalloc *sa = container_of (iface, stdalloc, alloc);
  stdalloc_assert (sa);

  // multiplication overflow check
  if (!can_mul_u64 (nmemb, size))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  u64 bytes = nmemb * size;

  // header and total checks
  if (!can_add_u64 (bytes, ALLOC_HEADER))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  bytes += ALLOC_HEADER;
  if (!can_add_u64 (sa->total, bytes))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  u64 new_total = sa->total + bytes;
  if (sa->limit > 0 && new_total > sa->limit)
    {
      return (alloc_ret){ .ret = ERR_OVERFLOW };
    }

  // system zeroed allocation
  u64 *p = calloc ((size_t)1, (size_t)bytes);
  if (p == NULL)
    {
      if (errno == ENOMEM)
        {
          return (alloc_ret){ .ret = ERR_OVERFLOW };
        }
      else
        {
          i_log_error ("calloc failed for %" PRIu64 " bytes: %s", bytes, strerror (errno));
          return (alloc_ret){ .ret = ERR_IO };
        }
    }

  // store header and return data pointer
  alloc_header_set (&p[1], bytes);
  sa->total = new_total;
  return (alloc_ret){ .ret = SUCCESS, .data = &p[1] };
}

static alloc_ret
stdalloc_realloc (lalloc *iface, void *data, u64 bytes)
{
  lalloc_assert (iface);
  ASSERT (data != NULL && bytes > 0);
  stdalloc *sa = container_of (iface, stdalloc, alloc);
  stdalloc_assert (sa);

  // header overflow check
  if (!can_add_u64 (bytes, ALLOC_HEADER))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  u64 *oldp = alloc_header_ptr (data);
  u64 old_size = alloc_header_get (data);

  // adjust total: remove old, add new
  if (!can_sub_u64 (sa->total, old_size) || !can_add_u64 (sa->total - old_size, bytes + ALLOC_HEADER))
    {
      return (alloc_ret){ .ret = ERR_ARITH };
    }
  u64 new_total = sa->total - old_size + (bytes + ALLOC_HEADER);
  if (sa->limit > 0 && new_total > sa->limit)
    {
      return (alloc_ret){ .ret = ERR_OVERFLOW };
    }

  // system reallocation
  u64 *p = realloc (oldp, (size_t)(bytes + ALLOC_HEADER));
  if (p == NULL)
    {
      if (errno == ENOMEM)
        {
          return (alloc_ret){ .ret = ERR_OVERFLOW };
        }
      else
        {
          i_log_error ("realloc failed for %" PRIu64 " bytes: %s", bytes, strerror (errno));
          return (alloc_ret){ .ret = ERR_IO };
        }
    }

  // update header and total
  alloc_header_set (&p[1], bytes + ALLOC_HEADER);
  sa->total = new_total;
  return (alloc_ret){ .ret = SUCCESS, .data = &p[1] };
}

static void
stdalloc_free (lalloc *iface, void *data)
{
  ASSERT (data != NULL);
  stdalloc *sa = container_of (iface, stdalloc, alloc);
  stdalloc_assert (sa);

  u64 size = alloc_header_get (data); // retrieve stored size
  ASSERT (can_sub_u64 (sa->total, size));
  sa->total -= size;
  free (alloc_header_ptr (data)); // free including header
}

static err_t
stdalloc_release (lalloc *iface)
{
  stdalloc *sa = container_of (iface, stdalloc, alloc);
  ASSERT (sa->total == 0);
  return SUCCESS;
}

stdalloc
stdalloc_create (u64 limit)
{
  return (stdalloc){
    .total = 0,
    .limit = limit,
    .alloc = {
        .malloc = stdalloc_malloc,
        .calloc = stdalloc_calloc,
        .realloc = stdalloc_realloc,
        .free = stdalloc_free,
        .release = stdalloc_release,
    }
  };
}

TEST (stdalloc)
{
  // limited allocator
  stdalloc sat = stdalloc_create (100);
  lalloc *alt = &sat.alloc;
  alloc_ret r1, r2;
  u8 *pt1, *pt2;

  r1 = alt->malloc (alt, 10);
  test_assert_int_equal (r1.ret, SUCCESS);
  test_assert_int_equal ((int)sat.total, ALLOC_HEADER + 10);
  pt1 = r1.data;

  r2 = alt->malloc (alt, U64_MAX);
  test_assert_int_equal (r2.ret, ERR_ARITH);

  r1 = alt->calloc (alt, 2, 5);
  test_assert_int_equal (r1.ret, SUCCESS);
  test_assert_int_equal ((int)sat.total, ALLOC_HEADER + 10 + ALLOC_HEADER + 2 * 5);
  pt2 = r1.data;

  r2 = alt->calloc (alt, U64_MAX, 2);
  test_assert_int_equal (r2.ret, ERR_ARITH);

  r1 = alt->realloc (alt, pt1, 5);
  test_assert_int_equal (r1.ret, SUCCESS);
  test_assert_int_equal ((int)sat.total, ALLOC_HEADER + 5 + ALLOC_HEADER + 2 * 5);

  r2 = alt->realloc (alt, pt1, U64_MAX);
  test_assert_int_equal (r2.ret, ERR_ARITH);

  alt->free (alt, pt2);
  test_assert_int_equal ((int)sat.total, ALLOC_HEADER + 5);
  alt->free (alt, pt1);
  test_assert_int_equal ((int)sat.total, 0);

  test_assert_int_equal (alt->release (alt), SUCCESS);

  stdalloc sau = stdalloc_create (0);
  lalloc *alu = &sau.alloc;

  r1 = alu->malloc (alu, U64_MAX);
  test_assert_int_equal (r1.ret, ERR_ARITH);

  r1 = alu->calloc (alu, 2, 5);
  test_assert_int_equal (r1.ret, SUCCESS);
  test_assert_int_equal ((int)sau.total, ALLOC_HEADER + 2 * 5);
  pt1 = r1.data;

  r1 = alu->realloc (alu, pt1, 5);
  test_assert_int_equal (r1.ret, SUCCESS);
  test_assert_int_equal ((int)sau.total, ALLOC_HEADER + 5);

  alu->free (alu, pt1);
  test_assert_int_equal ((int)sau.total, 0);

  test_assert_int_equal (alu->release (alu), SUCCESS);
}
