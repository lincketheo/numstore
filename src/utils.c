#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int file_create_from(const char* fname, void* data, usize lenb)
{
  int fd = open(fname, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("Failed to open file");
    return -1;
  }

  ssize_t written = write(fd, data, lenb);
  if (written != lenb) {
    perror("Failed to write complete data to file");
    close(fd);
    return -2;
  }

  return fd;
}

int write_all(int fd, const void* src, usize blen)
{
  assert(src);
  assert(blen > 0);
  assert(fd >= 0);

  ssize_t size = blen;
  ssize_t res = 0;
  const char* buffer = (const char*)src;

  while (size > 0) {
    res = write(fd, buffer, size);
    if (res == -1) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    assert(res <= size);

    size -= res;
    buffer += res;
  }

  return 0;
}

ssize read_all(int fd, void* dest, usize blen)
{
  assert(dest != NULL);
  assert(fd >= 0);

  ssize_t total_read = 0;
  ssize_t res = 0;
  char* buffer = (char*)dest;

  while (blen > 0) {
    res = read(fd, buffer, blen);
    if (res == -1) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }

    if (res == 0) {
      break;
    }

    assert(res <= blen);

    total_read += res;
    blen -= res;
    buffer += res;
  }

  return total_read;
}

///////// BYTES
void pretty_print_bytes(FILE* ofp, u8* data, usize len)
{
  const usize bytes_per_line = 16;

  for (usize i = 0; i < len; i += bytes_per_line) {
    printf("%08zx: ", i);

    for (usize j = 0; j < bytes_per_line; j++) {
      if (i + j < len) {
        printf("%02x ", data[i + j]);
      } else {
        printf("   ");
      }
    }

    printf(" |");
    for (usize j = 0; j < bytes_per_line; j++) {
      if (i + j < len) {
        u8 ch = data[i + j];
        printf("%c", isprint(ch) ? ch : '.');
      }
    }
    printf("|\n");
  }
}
