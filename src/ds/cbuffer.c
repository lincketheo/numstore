#include "ds/cbuffer.h"

#include "dev/assert.h"
#include "dev/testing.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"
#include "utils/macros.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

///////////////////////// Utils
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
      len = b->head - b->tail;
    }
  else
    {
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

void
cbuffer_discard_all (cbuffer *b)
{
  cbuffer_assert (b);
  b->tail = 0;
  b->head = 0;
  b->isfull = 0;
}

///////////////////////// Raw Read / Write

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

  // dest can be NULL, and it just blindly consumes
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
  test_assert_int_equal (cbuffer_len (&b), 1);
}

u32
cbuffer_copy (void *dest, u32 size, u32 n, const cbuffer *b)
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

  // Literally the exact same as read but
  // use these temp variables instead
  // I was lazy
  u32 tail = b->tail;
  bool isfull = b->isfull;

  while (bread < btoread)
    {
      u32 next;

      if (!isfull && b->head > tail)
        {
          next = MIN (b->head - tail, btoread - bread);
        }
      else
        {
          next = MIN (b->cap - tail, btoread - bread);
        }

      if (output)
        {
          i_memcpy (output + bread, b->data + tail, next);
        }
      tail = (tail + next) % b->cap;
      bread += next;
      isfull = 0;
    }

  ASSERT (ntoread * size == bread);
  return ntoread;
}

TEST (cbuffer_copy)
{
  u8 buf[3];
  cbuffer b = cbuffer_create (buf, 3);
  u8 out[3];

  // read from empty: returns 0
  u32 r1 = cbuffer_copy (out, 1, 1, &b);
  test_assert_int_equal (r1, 0);

  // read after write
  u8 src[3] = { 7, 8, 9 };
  cbuffer_write (src, 1, 3, &b);
  u32 r2 = cbuffer_copy (out, 1, 2, &b);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 7);
  test_assert_int_equal (out[1], 8);
  test_assert_int_equal (cbuffer_len (&b), 3);

  // Do it again and get the same results
  r2 = cbuffer_copy (out, 1, 2, &b);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 7);
  test_assert_int_equal (out[1], 8);
  test_assert_int_equal (cbuffer_len (&b), 3);
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

///////////////////////// CBuffer Read / Write

u32
cbuffer_cbuffer_move (cbuffer *dest, u32 sz, u32 cnt, cbuffer *src)
{
  cbuffer_assert (dest);
  cbuffer_assert (src);
  ASSERT (sz > 0 && cnt > 0);

  // calculate number of elements to move
  u32 src_elems = cbuffer_len (src) / sz;
  u32 dst_space = cbuffer_avail (dest) / sz;
  u32 n;
  if (cnt < src_elems)
    {
      n = cnt;
    }
  else
    {
      n = src_elems;
    }
  if (dst_space < n)
    {
      n = dst_space;
    }
  if (n == 0)
    {
      return 0;
    }

  // total bytes to transfer
  u32 bytes = n * sz;

  // first chunk: from tail up to end of buffer
  u32 first;
  if (src->isfull || src->head <= src->tail)
    {
      first = src->cap - src->tail;
    }
  else
    {
      first = src->head - src->tail;
    }
  if (first > bytes)
    {
      first = bytes;
    }

  // write first chunk into dest
  cbuffer_write (src->data + src->tail, 1, first, dest);

  // advance tail and clear full flag
  src->tail = (src->tail + first) % src->cap;
  src->isfull = false;
  bytes -= first;

  // second chunk if any remains
  if (bytes > 0)
    {
      cbuffer_write (src->data + src->tail, 1, bytes, dest);
      src->tail = (src->tail + bytes) % src->cap;
    }

  return n;
}

TEST (cbuffer_cbuffer_move)
{
  u8 buf_s[4];
  u8 buf_d[4];
  cbuffer src = cbuffer_create (buf_s, 4);
  cbuffer dst = cbuffer_create (buf_d, 4);
  u8 out[4];

  // empty source: should read 0 elements
  u32 r0 = cbuffer_cbuffer_move (&dst, 1, 1, &src);
  test_assert_int_equal (r0, 0);

  // fill source with 3 bytes
  u8 data2[3] = { 4, 5, 6 };
  cbuffer_write (data2, 1, 3, &src);

  // read 2 elements into dst
  u32 r1 = cbuffer_cbuffer_move (&dst, 1, 2, &src);
  test_assert_int_equal (r1, 2);
  test_assert_int_equal (cbuffer_len (&src), 1);
  test_assert_int_equal (cbuffer_len (&dst), 2);

  // verify dst contents
  u32 r2 = cbuffer_read (out, 1, 2, &dst);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 4);
  test_assert_int_equal (out[1], 5);
}
u32
cbuffer_cbuffer_copy (cbuffer *dest, u32 sz, u32 cnt, const cbuffer *src)
{
  cbuffer_assert (dest);
  cbuffer_assert (src);
  ASSERT (sz > 0 && cnt > 0);

  // calculate number of elements to copy
  u32 src_elems = cbuffer_len (src) / sz;
  u32 dst_space = cbuffer_avail (dest) / sz;
  u32 n;
  if (cnt < src_elems)
    {
      n = cnt;
    }
  else
    {
      n = src_elems;
    }
  if (dst_space < n)
    {
      n = dst_space;
    }
  if (n == 0)
    {
      return 0;
    }

  // total bytes to copy
  u32 bytes = n * sz;

  // local cursor and copy of full flag
  u32 pos = src->tail;
  bool full = src->isfull;

  // first chunk: from pos up to end of buffer
  u32 first;
  if (full || src->head <= pos)
    {
      first = src->cap - pos;
    }
  else
    {
      first = src->head - pos;
    }
  if (first > bytes)
    {
      first = bytes;
    }

  // write first chunk into dest
  cbuffer_write (src->data + pos, 1, first, dest);

  // advance local cursor
  pos = (pos + first) % src->cap;
  full = false;
  bytes -= first;

  // second chunk if any remains
  if (bytes > 0)
    {
      cbuffer_write (src->data + pos, 1, bytes, dest);
    }

  return n;
}
TEST (cbuffer_cbuffer_copy)
{
  u8 buf_s[4];
  u8 buf_d[4];
  cbuffer src = cbuffer_create (buf_s, 4);
  cbuffer dst = cbuffer_create (buf_d, 4);
  u8 out[4];

  // empty source: should copy 0 elements
  u32 r0 = cbuffer_cbuffer_copy (&dst, 1, 1, &src);
  test_assert_int_equal (r0, 0);

  // fill source with 3 bytes
  u8 data1[3] = { 1, 2, 3 };
  cbuffer_write (data1, 1, 3, &src);

  // copy 2 elements into dst
  u32 r1 = cbuffer_cbuffer_copy (&dst, 1, 2, &src);
  test_assert_int_equal (r1, 2);
  test_assert_int_equal (cbuffer_len (&src), 3);
  test_assert_int_equal (cbuffer_len (&dst), 2);

  // verify dst contents
  u32 r2 = cbuffer_read (out, 1, 2, &dst);
  test_assert_int_equal (r2, 2);
  test_assert_int_equal (out[0], 1);
  test_assert_int_equal (out[1], 2);
}

///////////////////////// IO Read / Write

i32
cbuffer_write_some_from_file (i_file *src, cbuffer *b, error *e)
{
  cbuffer_assert (b);
  ASSERT (src);

  u32 btowrite = cbuffer_avail (b);
  u32 bwrite = 0;

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

      i64 read = i_read_some (src, b->data + b->head, next, e);
      if (read < 0)
        {
          return err_t_from (e);
        }

      b->head = (b->head + read) % b->cap;
      bwrite += read;

      if (read < next)
        {
          // File is done
          return bwrite;
        }

      if (b->head == b->tail)
        {
          b->isfull = 1;
        }
    }

  return bwrite;
}

i32
cbuffer_read_some_to_file (i_file *dest, cbuffer *b, error *e)
{
  cbuffer_assert (b);

  u32 btoread = cbuffer_len (b);
  u32 bread = 0;

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

      i64 written = i_write_some (dest, b->data + b->tail, next, e);
      if (written < 0)
        {
          return err_t_from (e);
        }

      b->tail = (b->tail + written) % b->cap;
      bread += written;
      b->isfull = 0;

      if (written < next)
        {
          return bread;
        }
    }

  return btoread;
}

///////////////////////// Working with Single Elements
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

bool
cbuffer_enqueue (cbuffer *b, u8 val)
{
  cbuffer_assert (b);

  if (b->isfull)
    {
      return false;
    }

  b->data[b->head] = val;

  if (++b->head == b->cap)
    {
      b->head = false;
    }

  if (b->head == b->tail)
    {
      b->isfull = true;
    }

  return true;
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

bool
cbuffer_dequeue (u8 *dest, cbuffer *b)
{
  ASSERT (dest);
  cbuffer_assert (b);

  if (!b->isfull && b->head == b->tail)
    {
      return false;
    }

  *dest = cbuffer_dequeue_no_check (b);

  return true;
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
