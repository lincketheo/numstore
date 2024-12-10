#include "ns_buffer.h"
#include "ns_bytes.h"
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

TEST (buf_avail, ({
        char data[100];
        buf b = buf_create_from (data, 100, 0);
        TEST_EQUAL (buf_avail (b), 100lu, "%zu");

        const char *towrite = "hello world";
        buf c = buf_create_from (towrite, strlen (towrite) + 1,
                                 strlen (towrite) + 1);

        buf_move_buf (&b, &c, ssrange_from_num (c.len));

        TEST_EQUAL (buf_avail (b), 100lu - strlen (towrite) + 1, "%zu");
        TEST_ASSERT_STR_EQUAL ((char *)b.data, towrite);
      }));

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

  ns_memmove (&dest, &src, tomove * b.size);
  b.len = tomove;

  return b;
}

void
buf_move (buf *dest, buf *src, ssrange s)
{
  ASSERT (dest);
  ASSERT (src);
  ssrange_ASSERT (s);

  bytes _dest = buftobytesp (dest);
  bytes _src = buftobytesp (src);

  if (s.stride == 1)
    {
      ns_size tomove = MIN (buf_avail (*dest), src->len);
      ns_memmove (&_dest, &_src, tomove);
    }
}

void
buf_append (buf *dest, buf *src, ssrange s)
{
  ASSERT (dest);
  ASSERT (src);
  ssrange_ASSERT (s);
}
