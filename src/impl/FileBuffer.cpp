#include "impl/FileBuffer.hpp"
#include "testing.hpp"
#include "types.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

FileBuffer::FileBuffer (dtype type, int _fd) : Buffer (type) { fd = _fd; }

FileBuffer::~FileBuffer () {}

result<usize>
FileBuffer::read (void *dest, usize dnelem, srange range) const
{
  srange_assert (&range);
  assert(dest);
  assert(dnelem);

  // Get the file size
  struct stat st;
  if (fstat (fd, &st) == -1)
  {
    perror("fstat");
    return err<usize> ();
  }

  // Number of elements
  assert (st.st_size % dtype_sizeof (type) == 0);
  usize nelem = st.st_size / dtype_sizeof(type);

  // Avoid 0 memcpy's
  if (nelem == 0 || nelem <= range.start) {
    return ok(0lu);
  }

  // Memory Map File
  void *mem = mmap (nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mem == MAP_FAILED)
  {
    perror ("mmap");
    return err<usize> ();
  }

  // Copy data into destination
  usize ret = srange_copy ((u8 *)dest, dnelem, (u8 *)mem,
                           st.st_size / dtype_sizeof (type), range,
                           dtype_sizeof (type));

  // Close resources
  if(munmap(mem, st.st_size)) {
    perror("munmap");
    return err<usize> ();
  }

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
    .end = USIZE_MAX,
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

result<void>
FileBuffer::append (const void *data, usize nelem) const
{
  assert(data);
  assert(nelem > 0);

  if(lseek(fd, 0, SEEK_END) == -1) {
    perror("lseek");
    return err<void>();
  }

  if(write_all(fd, data, nelem * dtype_sizeof(type))) {
    return err<void>();
  }

  return ok_void();
}
