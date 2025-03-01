#include "Dataset.hpp"
#include "ranges.hpp"
#include "testing.hpp"
#include "types.hpp"

#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

Dataset::Dataset (dtype _type) : type (_type) {}

Dataset::~Dataset () {}

FileDataset::FileDataset (dtype type, int _fd) : Dataset (type)
{
  assert (_fd);
  fd = _fd;
}

static usize
srange_copy (u8 *dest, usize dnelem, const u8 *src, usize snelem, srange range,
             usize size)
{
  assert (dest);
  assert (src);
  assert (size > 0);

  usize di = 0, si = range.start;
  for (; si < range.end && si < snelem && di < dnelem; si += range.span, di++)
    {
      memcpy (dest + di * size, src + si * size, size);
    }
  return di;
}

TEST (srange_copy)
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

  usize copied = srange_copy ((u8 *)dest, sizeof (dest), (u8 *)src, 20, r,
                              sizeof (u32));

  test_assert_equal (copied, 9lu, "%zu");
  test_assert_equal (memcmp (dest, expect, 9 * sizeof (u32)), 0, "%d");
}

result<usize>
FileDataset::read (void *dest, usize dlen, srange range)
{
  srange_assert (&range);

  struct stat st;
  if (fstat (fd, &st) == -1)
    {
      return err<usize> ();
    }

  assert (st.st_size % dtype_sizeof (type) == 0);

  void *mem = mmap (nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  usize ret = srange_copy ((u8 *)dest, dlen, (u8 *)mem, st.st_size, range,
                           dtype_sizeof (type));
  return ok (ret);
}

result<usize>
FileDataset::append (const void *src, usize nelem)
{
  if (lseek (fd, 0, SEEK_END) == -1)
    {
      perror ("lseek");
      err<usize> ();
    }

  ssize written = write (fd, src, nelem * dtype_sizeof (type));
  if (written == -1)
    {
      perror ("write");
      err<usize> ();
    }

  return ok ((usize)written);
}
