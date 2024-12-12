#include "ns_buffer.h"

#include "ns_bytes.h"
#include "ns_macros.h"
#include "ns_srange.h"
#include "ns_testing.h"
#include "ns_utils.h"

#include <string.h>

buf
buf_create_empty_from_bytes (bytes b, ns_size size)
{
  ASSERT (b.cap % size == 0);

  return (buf){
    .data = b.data,
    .cap = b.cap / size,
    .size = size,
    .len = 0,
  };
}

ns_size
buf_avail (buf b)
{
  buf_ASSERT (&b);
  return b.cap - b.len;
}

TEST (buf_avail)
{
  char data[100];

  // At the start, our buffer is empty -
  // so it's capacity is what's available
  buf b = buf_create_from (data, 100, 0);
  TEST_EQUAL (buf_avail (b), 100lu, "%zu");

  // Create a new buffer from a string
  const char *towrite = "hello world";
  buf c = buf_create_from (towrite, strlen (towrite) + 1, strlen (towrite) + 1);

  // Write to the first buffer
  buf_move_buf (&b, &c, ssrange_from_num (c.len));

  // We should have cap - strlen available now
  TEST_EQUAL (buf_avail (b), 100lu - strlen (towrite) - 1, "%zu");
  TEST_ASSERT_STR_EQUAL ((char *)b.data, towrite);
}

buf
buf_shift_mem (buf b, ns_size ind)
{
  buf_ASSERT (&b);
  ASSERT (ind <= b.len);

  if (ind == b.len)
  {
    b.len = 0;
    return b;
  }

  ns_size tomove = b.len - ind;

  bytes dest = bytes_create_from (b.data, b.cap * b.size);
  bytes src = bytes_from (dest, ind * b.size);

  bytes_memmove (&dest, &src, tomove * b.size);
  b.len = tomove;

  return b;
}

TEST(buf_shift_mem) {
  int data[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  size_t cap = sizeof(data) / sizeof(int);

  // At the start, our buffer is empty
  // so it's capacity is what's available
  buf b = buf_create_full_from (data, cap);
  TEST_EQUAL(b.len, cap, "%zu");
  TEST_EQUAL(((int*)b.data)[0], 0, "%d");
  TEST_EQUAL(((int*)b.data)[cap - 1], 7, "%d");
  b = buf_shift_mem(b, 1);
  TEST_EQUAL(b.len, cap - 1, "%zu");
  TEST_EQUAL(((int*)b.data)[0], 1, "%d");
  TEST_EQUAL(((int*)b.data)[cap - 2], 7, "%d");
  b = buf_shift_mem(b, 1);
  TEST_EQUAL(b.len, cap - 2, "%zu");
  TEST_EQUAL(((int*)b.data)[0], 2, "%d");
  TEST_EQUAL(((int*)b.data)[cap - 3], 7, "%d");
  b = buf_shift_mem(b, 4);
  TEST_EQUAL(b.len, cap - 6, "%zu");
  TEST_EQUAL(((int*)b.data)[0], 6, "%d");
  TEST_EQUAL(((int*)b.data)[cap - 7], 7, "%d");
}

void
buf_move_buf (buf *d, buf *s, ssrange r)
{
  buf_ASSERT(d);
  buf_ASSERT(s);
  ssrange_ASSERT(r);
  ASSERT(d->size == s->size);

  ns_size si = r.start;

  while(1) {
    buf_ASSERT(d);
    buf_ASSERT(s);

    if(si + r.stride > s->len) {
      break;
    }
    if(si + r.stride > r.end) {
      break;
    }
    if(d->len == d->cap) {
      break;
    }

    memcpy(&d->data[d->len * d->size], &s->data[si * s->size], s->size);

    d->len++;
    si += r.stride;

    *s = buf_shift_mem(*s, si);
  }
}

int bufcmp(buf a, buf b) {
  buf_ASSERT(&a);
  buf_ASSERT(&b);
  ASSERT(b.size == a.size);

  if(b.len != a.len) {
    return 0;
  }

  return memcmp(a.data, b.data, a.size * a.len) == 0;
}

TEST(buf_move_buf) {
  int src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int dest[20] = {-1, -2, -3};

  buf sbuf = buf_create_full_from(src, arr_sizeof(src));
  buf dbuf = buf_create_from(dest, arr_sizeof(dest), 3);

  int sexp1[] = {3, 4, 5, 6, 7, 8, 9, 10};
  int dexp1[] = {-1, -2, -3, 1, 2};

  buf sexp = buf_create_full_from(sexp1, arr_sizeof(sexp1));
  buf dexp = buf_create_full_from(dexp1, arr_sizeof(dexp1));

  buf_move_buf(&dbuf, &sbuf, (ssrange){ 0, 2, 1, 0 });
  TEST_EQUAL(dbuf.len, 5lu, "%zu");
  TEST_EQUAL(bufcmp(dbuf, dexp), 1, "%d");
  TEST_EQUAL(bufcmp(sbuf, sexp), 1, "%d");

  int sexp2[] = { 10 };
  int dexp2[] = { -1, -2, -3, 1, 2, 3, 5, 7, 9 };

  sexp = buf_create_full_from(sexp2, arr_sizeof(sexp2));
  dexp = buf_create_full_from(dexp2, arr_sizeof(dexp2));

  buf_move_buf(&dbuf, &sbuf, (ssrange){ 0, 100, 2, 0 });
  TEST_EQUAL(dbuf.len, 9lu, "%zu");
  TEST_EQUAL(bufcmp(dbuf, dexp), 1, "%d");
  TEST_EQUAL(bufcmp(sbuf, sexp), 1, "%d");
}

void
buf_append (buf *dest, buf *src, ssrange s)
{
  ASSERT (dest);
  ASSERT (src);
  ssrange_ASSERT (s);
}
