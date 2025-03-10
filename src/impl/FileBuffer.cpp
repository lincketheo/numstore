#include "impl/FileBuffer.hpp"
#include "testing.hpp"
#include "utils.hpp"

#include <cassert>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

FileBuffer::FileBuffer (dtype type, int _fd) : Buffer (type) { fd = _fd; }

FileBuffer::~FileBuffer () {}

result<usize>
FileBuffer::read (void *dest, usize dnelem, srange range)
{
  srange_assert (&range);

  struct stat st;
  if (fstat (fd, &st) == -1)
    {
      return err<usize> ();
    }

  assert (st.st_size % dtype_sizeof (type) == 0);

  void *mem = mmap (nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  usize ret = srange_copy ((u8 *)dest, dnelem, (u8 *)mem,
                           st.st_size / dtype_sizeof (type), range,
                           dtype_sizeof (type));
  return ok (ret);
}

TEST (MemoryBuffer_read)
{
  u32 dest[10];
  u32 src[20] = { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  u32 expect[10] = { 3, 5, 7, 9, 11, 13, 15, 17, 19 };

  int destfd = file_create_from ("test.bin", src, sizeof (src));

  srange r = {
    .start = 2,
    .end = SIZE_T_MAX,
    .span = 2,
  };

  FileBuffer b (U32, destfd);

  result<usize> copied = b.read (dest, 10, r);
  test_assert_equal (copied.ok, 1, "%d");

  test_assert_equal (copied.value, 9lu, "%zu");
  test_assert_equal (memcmp (dest, expect, 9 * sizeof (u32)), 0, "%d");

  close (destfd);
  remove ("test.bin");
}
