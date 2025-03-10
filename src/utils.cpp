#include "utils.hpp"

#include <fcntl.h>
#include <unistd.h>

int
file_create_from (const char *fname, void *data, usize lenb)
{
  int fd = open (fname, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1)
    {
      perror ("Failed to open file");
      return -1;
    }

  ssize_t written = write (fd, data, lenb);
  if (written != lenb)
    {
      perror ("Failed to write complete data to file");
      close (fd);
      return -2;
    }

  return fd;
}
