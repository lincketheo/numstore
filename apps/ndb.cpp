#include "impl/FileBuffer.hpp"
#include "types.hpp"
#include <fcntl.h>
#include <unistd.h>

int
main ()
{
  u32 data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  usize nelem = sizeof (data) / sizeof (*data);

  int fd = open ("test.bin", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  assert (fd != -1);

  FileBuffer fb (U32, fd);
  fb.append (data, nelem);
  fb.append (data, nelem);

  // 1 2 3 4 5 6 7 8 9 10 1 2 3 4 5 6 7 8 9 10

  u32 dest[10];

  result<usize> ret = fb.read (dest, 10,
                               (srange){
                                   .start = 1,
                                   .end = 100,
                                   .span = 4,
                               });

  assert (ret.ok);

  for (int i = 0; i < ret.value; ++i)
    {
      printf ("%u\n", dest[i]);
    }

  return 0;
}
