#include "sds.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "types.h"
#include "utils/bounds.h"
#include "utils/macros.h"
#include "utils/numbers.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

//////////////////////// CBUFFER
DEFINE_DBG_ASSERT_I (cbuffer, cbuffer, b)
{
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull)
    {
      ASSERT (b->tail == b->head);
    }
  ASSERT (cbuffer_len (b) <= b->cap);
}

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

bool
cbuffer_isempty (const cbuffer *b)
{
  cbuffer_assert (b);
  return (!b->isfull && b->head == b->tail);
}

TEST (cbuffer_isempty)
{
  u8 buf[1];
  cbuffer b = cbuffer_create (buf, 1);
  test_assert_int_equal (cbuffer_isempty (&b), 1);
  cbuffer_enqueue (&b, 0xFF);
  test_assert_int_equal (cbuffer_isempty (&b), 0);
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
      ASSERT (can_sub_u32 (b->head, b->tail));
      len = b->head - b->tail;
    }
  else
    {
      ASSERT (can_sub_u32 (b->tail, b->head));
      ASSERT (can_sub_u32 (b->cap, b->tail - b->head));
      len = b->cap - (b->tail - b->head);
    }
  return len;
}

TEST (cbuffer_len)
{
  u8 buf[4];
  cbuffer b = cbuffer_create (buf, 4);
  test_assert_int_equal (cbuffer_len (&b), 0);
  cbuffer_enqueue (&b, 1);
  cbuffer_enqueue (&b, 2);
  test_assert_int_equal (cbuffer_len (&b), 2);
}

u32
cbuffer_avail (const cbuffer *b)
{
  cbuffer_assert (b);
  u32 len = cbuffer_len (b);
  ASSERT (b->cap >= len);
  return b->cap - len;
}

TEST (cbuffer_avail)
{
  u8 buf[4];
  cbuffer b = cbuffer_create (buf, 4);
  test_assert_int_equal (cbuffer_avail (&b), 4);
  cbuffer_enqueue (&b, 9);
  test_assert_int_equal (cbuffer_avail (&b), 3);
}

u32
cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b)
{
  cbuffer_assert (b);
  ASSERT (size > 0);
  ASSERT (n > 0);

  u32 len = cbuffer_len (b) / size;
  u32 ntoread = (n < len) ? n : len;
  u32 btoread = ntoread * size;
  u32 bread = 0;

  u8 *output = NULL;
  if (dest)
    {
      output = (u8 *)dest;
    }

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

      if (output)
        {
          i_memcpy (output + bread, b->data + b->tail, next);
        }
      b->tail = (b->tail + next) % b->cap;
      bread += next;
      b->isfull = 0;
    }

  ASSERT (ntoread * size == bread);
  return ntoread;
}

TEST (cbuffer_read)
{
  u8 buf[3];
  cbuffer b = cbuffer_create (buf, 3);
  u8 out[3];

  // read from empty: returns 0
  u32 r1 = cbuffer_read (out, 1, 1, &b);
  test_assert_int_equal (r1, 0);

  // read after write
  u8 src[3] = { 7, 8, 9 };
  cbuffer_write (src, 1, 3, &b);
  u32 r2 = cbuffer_read (out, 1, 2, &b);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 7);
  test_assert_int_equal (out[1], 8);
}

u32
cbuffer_write (const void *src, u32 size, u32 n, cbuffer *b)
{
  cbuffer_assert (b);
  ASSERT (src);
  ASSERT (size > 0);
  ASSERT (n > 0);

  ASSERT (b->cap >= cbuffer_len (b));
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

  ASSERT (ntowrite * size == bwrite);
  return ntowrite;
}

TEST (cbuffer_write)
{
  u8 buf[3];
  cbuffer b = cbuffer_create (buf, 3);
  u8 src[4] = { 1, 2, 3, 4 };

  // size > capacity: writes nothing
  u32 w1 = cbuffer_write (src, 4, 1, &b);
  test_assert_int_equal (w1, 0);

  // element size 1, too many elements: only 3 fit
  u32 w2 = cbuffer_write (src, 1, 4, &b);
  test_assert_int_equal (w2, 3);
  test_assert_int_equal (cbuffer_len (&b), 3);
}

static inline u8
cbuffer_get_no_check (const cbuffer *b, int idx)
{
  ASSERT (idx >= 0);
  cbuffer_assert (b);
  return b->data[(b->tail + (u32)idx) % b->cap];
}

static inline u8
cbuffer_peek_dequeue_no_check (const cbuffer *b)
{
  cbuffer_assert (b);
  return b->data[b->tail];
}

static inline u8
cbuffer_dequeue_no_check (cbuffer *b)
{
  cbuffer_assert (b);

  u8 ret = b->data[b->tail];
  b->isfull = 0;
  b->tail = (b->tail + 1) % b->cap;
  return ret;
}

bool
cbuffer_get (u8 *dest, const cbuffer *b, int idx)
{
  ASSERT (idx >= 0);
  cbuffer_assert (b);
  ASSERT (dest);

  if (!b->isfull && (u32)idx >= (b->head + b->cap - b->tail) % b->cap)
    {
      return 0;
    }

  *dest = cbuffer_get_no_check (b, idx);
  return 1;
}

TEST (cbuffer_get)
{
  u8 buf[2];
  cbuffer b = cbuffer_create (buf, 2);
  cbuffer_enqueue (&b, 5);
  u8 v;
  // in-range index
  test_assert_int_equal (cbuffer_get (&v, &b, 0), 1);
  test_assert_int_equal (v, 5);
  // out-of-range index
  test_assert_int_equal (cbuffer_get (&v, &b, 1), 0);
}

bool
cbuffer_peek_dequeue (u8 *dest, const cbuffer *b)
{
  ASSERT (dest);
  cbuffer_assert (b);

  if (!b->isfull && b->head == b->tail)
    {
      return 0;
    }

  *dest = cbuffer_peek_dequeue_no_check (b);
  return 1;
}

TEST (cbuffer_peek_dequeue)
{
  u8 buf1[1];
  cbuffer b1 = cbuffer_create (buf1, 1);
  u8 v;

  // peek on empty buffer fails
  test_assert_int_equal (cbuffer_peek_dequeue (&v, &b1), 0);

  // enqueue then peek non-empty
  cbuffer_enqueue (&b1, 0xAA);
  test_assert_int_equal (cbuffer_peek_dequeue (&v, &b1), 1);
  test_assert_int_equal (v, 0xAA);

  // peek does not remove element: length remains 1
  test_assert_int_equal (cbuffer_len (&b1), 1);

  // dequeue and then peek fails again
  cbuffer_dequeue (&v, &b1);
  test_assert_int_equal (cbuffer_peek_dequeue (&v, &b1), 0);

  // wrap-around peek: fill, dequeue twice, enqueue new
  u8 buf2[3];
  cbuffer b2 = cbuffer_create (buf2, 3);
  cbuffer_enqueue (&b2, 5);
  cbuffer_enqueue (&b2, 6);
  cbuffer_enqueue (&b2, 7); // head wraps to 0, isfull=1

  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1); // removes 5
  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1); // removes 6

  cbuffer_enqueue (&b2, 8); // writes 8 at wrapped head
  // peek front element after wrap-around
  test_assert_int_equal (cbuffer_peek_dequeue (&v, &b2), 1);
  test_assert_int_equal (v, 7);
}

int
cbuffer_enqueue (cbuffer *b, u8 val)
{
  cbuffer_assert (b);

  if (b->isfull)
    {
      return 0;
    }

  b->data[b->head] = val;

  if (++b->head == b->cap)
    {
      b->head = 0;
    }

  if (b->head == b->tail)
    {
      b->isfull = 1;
    }

  return 1;
}

TEST (cbuffer_enqueue)
{
  u8 buf[2];
  cbuffer b = cbuffer_create (buf, 2);
  u8 v;

  // enqueue into empty buffer succeeds
  test_assert_int_equal (cbuffer_enqueue (&b, 0x11), 1);

  // enqueue into nearly-full buffer succeeds
  test_assert_int_equal (cbuffer_enqueue (&b, 0x22), 1);

  // buffer is full now, next enqueue fails
  test_assert_int_equal (cbuffer_enqueue (&b, 0x33), 0);

  // dequeue one element to free space
  test_assert_int_equal (cbuffer_dequeue (&v, &b), 1);
  test_assert_int_equal (v, 0x11);

  // enqueue after dequeue should succeed and wrap head into freed slot
  test_assert_int_equal (cbuffer_enqueue (&b, 0x33), 1);

  // buffer full again, further enqueues still fail
  test_assert_int_equal (cbuffer_enqueue (&b, 0x44), 0);
}

int
cbuffer_dequeue (u8 *dest, cbuffer *b)
{
  ASSERT (dest);
  cbuffer_assert (b);

  if (!b->isfull && b->head == b->tail)
    {
      return 0;
    }

  *dest = cbuffer_dequeue_no_check (b);

  return 1;
}

TEST (cbuffer_dequeue)
{
  u8 buf1[1];
  cbuffer b1 = cbuffer_create (buf1, 1);
  u8 v;

  // dequeue on empty buffer fails
  test_assert_int_equal (cbuffer_dequeue (&v, &b1), 0);

  // enqueue then dequeue returns value
  cbuffer_enqueue (&b1, 0x7F);
  test_assert_int_equal (cbuffer_dequeue (&v, &b1), 1);
  test_assert_int_equal (v, 0x7F);

  // subsequent dequeue fails when empty again
  test_assert_int_equal (cbuffer_dequeue (&v, &b1), 0);

  // wrap-around: fill, dequeue one, then enqueue new
  u8 buf2[3];
  cbuffer b2 = cbuffer_create (buf2, 3);
  cbuffer_enqueue (&b2, 1);
  cbuffer_enqueue (&b2, 2);
  cbuffer_enqueue (&b2, 3); // head wraps to 0, isfull=1

  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1); // gets 1
  test_assert_int_equal (v, 1);
  cbuffer_enqueue (&b2, 4); // stores 4 at wrapped head
  // dequeue remaining in order: 2,3,4
  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1);
  test_assert_int_equal (v, 2);
  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1);
  test_assert_int_equal (v, 3);
  test_assert_int_equal (cbuffer_dequeue (&v, &b2), 1);
  test_assert_int_equal (v, 4);
}

bool
cbuffer_strequal (const cbuffer *c, const char *str, u32 strlen)
{
  ASSERT (str);
  ASSERT (strlen > 0);
  cbuffer_assert (c);

  u8 ch;
  for (u32 i = 0; i < strlen; ++i)
    {
      ASSERT (is_alpha_num (str[i]));
      if (!cbuffer_get (&ch, c, i))
        {
          return false;
        }
      if (ch != str[i])
        {
          return false;
        }
    }

  return true;
}

TEST (cbuffer_strequal)
{
  u8 buf1[4];
  cbuffer b1 = cbuffer_create (buf1, 4);
  // empty buffer vs non-empty string
  test_assert_int_equal (cbuffer_strequal (&b1, "A", 1), 0);

  const char *s = "ABCX";
  cbuffer_write (s, 1, 4, &b1);
  test_assert_int_equal (cbuffer_strequal (&b1, "ABC", 3), 1);
  test_assert_int_equal (cbuffer_strequal (&b1, "ABD", 3), 0);

  // wrap‑around: capacity 4, write 4, read 2, write 2
  cbuffer_read (NULL, 1, 2, &b1);
  const char *t = "YD";
  cbuffer_write (t, 1, 2, &b1);
  test_assert_int_equal (cbuffer_strequal (&b1, "CXY", 3), 1);
  test_assert_int_equal (cbuffer_strequal (&b1, "CXYD", 4), 1);
  test_assert_int_equal (cbuffer_strequal (&b1, "CXZ", 3), 0);
}

//////////////////////// BYTES

DEFINE_DBG_ASSERT_I (bytes, bytes, b)
{
  ASSERT (b);
  ASSERT (b->data);
  ASSERT (b->cap > 0);
}

bytes
bytes_create (u8 *data, u32 cap)
{
  ASSERT (data);
  ASSERT (cap > 0);
  return (bytes){
    .data = data,
    .cap = cap,
  };
}

u32
bytes_write (const void *dest, u32 size, u32 n, bytes *b)
{
  ASSERT (dest);
  ASSERT (size > 0);
  ASSERT (n > 0);
  bytes_assert (b);

  u32 ncap = b->cap / size;
  u32 ret = MIN (size * n, size * ncap);
  i_memcpy (b->data, dest, ret);

  return ret;
}

TEST (bytes_write)
{
  u8 dst[8] = { 0 };
  const u8 src[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
  bytes b = bytes_create (dst, 8);

  // normal write: size=1, n=5 => should write 5 bytes
  u32 w1 = bytes_write (src, 1, 5, &b);
  test_assert_int_equal (w1, 5);
  for (int i = 0; i < 5; ++i)
    {
      test_assert_equal ((int)dst[i], (int)src[i], "%d");
    }
  // bytes beyond written length remain zero
  for (int i = 5; i < 8; ++i)
    {
      test_assert_equal ((int)dst[i], 0, "%d");
    }

  // n exceeds capacity: size=1, n=20 => writes up to cap (8 bytes)
  i_memset (dst, 0, sizeof dst);
  u32 w2 = bytes_write (src, 1, 20, &b);
  test_assert_int_equal (w2, 8);
  for (int i = 0; i < 8; ++i)
    {
      test_assert_equal ((int)dst[i], (int)src[i], "%d");
    }

  // element size > cap: size=16, n=1 => writes nothing
  u8 dst2[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
  const u8 src2[2] = { 1, 2 };
  bytes b2 = bytes_create (dst2, 4);
  u32 w3 = bytes_write (src2, 16, 1, &b2);
  test_assert_int_equal (w3, 0);
  // dst2 remains unchanged
  for (int i = 0; i < 4; ++i)
    {
      test_assert_equal ((int)dst2[i], 0xFF, "%#x");
    }
}

//////////////////////// STRINGS

DEFINE_DBG_ASSERT_I (string, string, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
}

DEFINE_DBG_ASSERT_I (string, cstring, s)
{
  string_assert (s);
  ASSERT (s->data[s->len] == 0);
}

string
unsafe_cstrfrom (char *cstr)
{
  return (string){
    .data = cstr,
    .len = i_unsafe_strlen (cstr)
  };
}

int
strings_all_unique (const string *strs, u32 count)
{
  for (u32 i = 0; i < count; ++i)
    {
      for (u32 j = i + 1; j < count; ++j)
        {
          if (strs[i].len != strs[j].len)
            {
              continue;
            }
          if (i_memcmp (strs[i].data, strs[j].data, strs[i].len) == 0)
            {
              return 0;
            }
        }
    }
  return 1;
}

TEST (strings_all_unique)
{
  // empty array: trivially unique
  test_assert_int_equal (strings_all_unique (NULL, 0), 1);

  // one string: unique
  char data1[] = "hello";
  string s1[] = { { data1, 5 } };
  test_assert_int_equal (strings_all_unique (s1, 1), 1);

  // two different strings, different lengths
  char d1[] = "a";
  char d2[] = "ab";
  string s2[] = { { d1, 1 }, { d2, 2 } };
  test_assert_int_equal (strings_all_unique (s2, 2), 1);

  // two different strings, same length
  char e1[] = "ab";
  char e2[] = "cd";
  string s3[] = { { e1, 2 }, { e2, 2 } };
  test_assert_int_equal (strings_all_unique (s3, 2), 1);

  // duplicate strings
  char f1[] = "dup";
  char f2[] = "dup";
  string s4[] = { { f1, 3 }, { f2, 3 } };
  test_assert_int_equal (strings_all_unique (s4, 2), 0);

  // multiple strings with one duplicate in the middle
  char g1[] = "one";
  char g2[] = "two";
  char g3[] = "one";
  char g4[] = "four";
  string s5[] = { { g1, 3 }, { g2, 3 }, { g3, 3 }, { g4, 4 } };
  test_assert_int_equal (strings_all_unique (s5, 4), 0);

  // all unique in larger set
  char h1[] = "aa";
  char h2[] = "bb";
  char h3[] = "cc";
  char h4[] = "dd";
  string s6[] = { { h1, 2 }, { h2, 2 }, { h3, 2 }, { h4, 2 } };
  test_assert_int_equal (strings_all_unique (s6, 4), 1);
}

bool
string_equal (const string s1, const string s2)
{
  string_assert (&s1);
  string_assert (&s2);
  if (s1.len != s2.len)
    {
      return false;
    }
  return i_strncmp (s1.data, s2.data, s1.len) == 0;
}

DEFINE_DBG_ASSERT_I (array_range, array_range, a)
{
  ASSERT (a);
  if (a->stop > a->start)
    {
      ASSERT (a->step > 0);
    }
  else
    {
      ASSERT (a->step < 0);
    }
}
