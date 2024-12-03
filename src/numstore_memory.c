#include "numstore_memory.h" // root

#include "common_assert.h"
#include "common_testing.h"

#include <errno.h>
#include <stdint.h>

// Probably need to increase
#define MAX_CAP 100000

lmcreate_ret
linmem_create (linmem *dest, size_t cap)
{
  ASSERT (dest);
  ASSERT (cap > 0);
  ASSERT (cap < MAX_CAP);

  errno = 0;
  dest->data = malloc (cap);
  if (dest->data == NULL)
    {
      return (lmcreate_ret){
        .type = LMC_MALLOC_FAILED,
        .errno_if_malloc_failed = errno,
      };
    }

  dest->cap = cap;
  dest->len = 0;

  return (lmcreate_ret){
    .type = LMC_SUCCESS,
  };
}

size_t
linmem_start_scope (linmem *m)
{
  ASSERT (m);
  return m->len;
}

void
linmem_reset (linmem *m, size_t scope)
{
  ASSERT (m);
  ASSERT (scope < m->len);
  m->len = scope;
}

void *
linmem_malloc (linmem *m, size_t len)
{
  if (m->len + len > m->cap)
    {
      return NULL;
    }

  void *ret = &((uint8_t *)m->data)[m->len];
  m->len += len;
  ASSERT (ret);
  return ret;
}

#ifndef NTEST
int
test_linmem_malloc ()
{
  linmem s;
  lmcreate_ret ret = linmem_create (&s, 100);
  TEST_EQUAL (ret.type, LMC_SUCCESS, "%d");

  char *head1 = linmem_malloc (&s, 5);
  memcpy (head1, "foob", 5);
  TEST_EQUAL (s.len, 5lu, "%zu");

  char *head2 = linmem_malloc (&s, 10);
  memcpy (head2, "foobarbaz", 10);
  TEST_EQUAL (s.len, 15lu, "%zu");

  TEST_ASSERT_STR_EQUAL ("foob", (char *)head1);
  TEST_ASSERT_STR_EQUAL ("foobarbaz", (char *)head2);
  linmem_free (&s);
  return 0;
}
REGISTER_TEST (test_linmem_malloc);
#endif

void
linmem_free (linmem *m)
{
  ASSERT (m);
  ASSERT (m->data);
  free (m->data);
  m->cap = 0;
  m->len = 0;
}
