#include "Dataset.hpp"
#include "types.hpp"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

int
main ()
{

  int fd = open ("example.txt", O_RDWR | O_CREAT, 0644);

  FileDataset f (U32, fd);

  u32 data[10];
  u32 dest[2048];
  for (int i = 0; i < 10; ++i)
    {
      data[i] = (u32)i;
    }

  f.append (data, 10);
  f.append (data, 10);

  result<usize> r = f.read (dest, 2048, { .start = 1, .span = 3, .end = 100 });
  if (!r.ok)
    {
      fprintf (stderr, "Failed");
      exit (-1);
    }

  for (int i = 0; i < r.value; ++i)
    {
      fprintf (stdout, "%u\n", dest[i]);
    }

  return 0;
}
