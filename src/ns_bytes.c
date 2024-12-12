#include "ns_bytes.h"
#include "ns_macros.h"
#include "ns_testing.h"

#include <string.h> // memcpy

///////////////////////////////
////////// Bytes

void
bytes_memcpy (bytes *dest, const bytes *src, ns_size size)
{
  ASSERT (dest);
  ASSERT (src);
  ASSERT (dest->cap >= size);
  ASSERT (src->cap >= size);
  memcpy (dest->data, src->data, size);
}

TEST(bytes_memcpy) {
  int src[] = {1, 2, 3, 4, 5};
  int dest[] = { 0, 0, 0, 0, 0 };
  ns_size slen = arr_sizeof(src);

  bytes s = bytes_create_from(src, 5 * sizeof *src);
  bytes d = bytes_create_from(dest, 5 * sizeof *dest);

  bytes_memcpy(&d, &s, 5 * sizeof*src);
  TEST_EQUAL(memcmp(d.data, s.data, d.cap), 0, "%d");
}

void
bytes_memmove (bytes *dest, const bytes *src, ns_size size)
{
  ASSERT (dest);
  ASSERT (src);
  ASSERT (dest->cap >= size);
  ASSERT (src->cap >= size);
  memmove (dest->data, src->data, size);
}

TEST(bytes_memmove) {
  int dest[] = {1, 2, 3, 4, 5};
  int *src = &dest[2];
  int exp[] = {3, 4, 5, 4, 5};
  ns_size slen = arr_sizeof(dest);

  bytes s = bytes_create_from(src, 3 * sizeof *src);
  bytes d = bytes_create_from(dest, 5 * sizeof *dest);
  bytes e = bytes_create_from(exp, 5 * sizeof *exp);

  bytes_memmove(&d, &s, 3 * sizeof*src);
  TEST_EQUAL(bytes_all_equal(d, e), true, "%d");
}

bytes
bytes_from (const bytes src, ns_size from)
{
  ASSERT (!bytes_IS_FREE (&src));
  ASSERT (from <= src.cap);
  return bytes_create_from (&src.data[from], src.cap - from);
}

TEST(bytes_from) {
  int data[] = {1, 2, 3, 4, 5};
  int exp[] = {3, 4, 5};

  bytes d = bytes_create_from(data, 5 * sizeof *data);
  bytes e = bytes_create_from(exp, 3 * sizeof *exp);

  bytes a = bytes_from(d, 2);
  TEST_EQUAL(bytes_all_equal(a, e), true, "%d");
}

ns_bool bytes_all_equal(const bytes left, const bytes right) {
  if(left.cap != right.cap) {
    return false;
  }
  return memcmp(left.data, right.data, right.cap) == 0;
}

TEST(bytes_all_equal) {
  int left[] = {1, 2, 3, 4, 5};
  int right[] = {1, 2, 3, 4, 5, 0};

  bytes _left = bytes_create_from(left, 5 * sizeof *left);
  bytes _right = bytes_create_from(right, 5 * sizeof *right);

  TEST_EQUAL(bytes_all_equal(_left, _right), true, "%d");
  _right = bytes_create_from(right, 6 * sizeof *right);
  TEST_EQUAL(bytes_all_equal(_left, _right), false, "%d");
  _right = bytes_create_from(right, 4 * sizeof *right);
  TEST_EQUAL(bytes_all_equal(_left, _right), false, "%d");

  right[0] = 5;

  TEST_EQUAL(bytes_all_equal(_left, _right), false, "%d");
}
