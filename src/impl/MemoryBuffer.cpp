#include "impl/MemoryBuffer.hpp"
#include "testing.hpp"

#include <cassert>
#include <sys/mman.h>
#include <sys/stat.h>

MemoryBuffer::MemoryBuffer (dtype type, void *_data, usize _nelem,
                            usize _capelem)
    : Buffer (type)
{
  assert (_nelem <= _capelem);
  data = _data;
  capelem = _capelem;
  nelem = _nelem;
}

MemoryBuffer::~MemoryBuffer () {}

result<usize>
MemoryBuffer::read (void *dest, usize dnelem, srange range) const
{
  srange_assert (&range);

  usize ret = srange_copy ((u8 *)dest, dnelem, (u8 *)data, nelem, range,
                           dtype_sizeof (type));
  return ok (ret);
}

TEST (MemoryBuffer_read)
{
  u32 dest[10];
  u32 src[20] = { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  u32 expect[10] = { 3, 5, 7, 9, 11, 13, 15, 17, 19 };

  srange r = {
    .start = 2,
    .end = SIZE_T_MAX,
    .span = 2,
  };

  MemoryBuffer b (U32, src, 20, 20);

  result<usize> copied = b.read (dest, 10, r);
  test_assert_equal (copied.ok, 1, "%d");

  test_assert_equal (copied.value, 9lu, "%zu");
  test_assert_equal (memcmp (dest, expect, 9 * sizeof (u32)), 0, "%d");
}

result<usize>
MemoryBuffer::append (const void *data, usize nelem) const
{
  return err<usize> ();
}
