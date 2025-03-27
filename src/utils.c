#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int file_create_from(const char *fname, void *data, u64 lenb) {
  int fd = open(fname, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    perror("Failed to open file");
    return -1;
  }

  ssize_t written = write(fd, data, lenb);
  if (written != (ssize_t)lenb) {
    perror("Failed to write complete data to file");
    close(fd);
    return -2;
  }

  return fd;
}

int write_all(int fd, const void *src, u64 blen) {
  assert(src);
  assert(blen > 0);
  assert(fd >= 0);

  ssize_t size = blen;
  ssize_t res = 0;
  const char *buffer = (const char *)src;

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

i64 read_all(int fd, void *dest, u64 blen) {
  assert(dest != NULL);
  assert(fd >= 0);

  ssize_t total_read = 0;
  ssize_t res = 0;
  char *buffer = (char *)dest;

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

    assert((u64)res <= blen);

    total_read += res;
    blen -= res;
    buffer += res;
  }

  return (i64)total_read;
}

///////// BYTES
void pretty_print_bytes(FILE *ofp, u8 *data, u64 len) {
  const u64 bytes_per_line = 16;

  for (u64 i = 0; i < len; i += bytes_per_line) {
    fprintf(ofp, "%08" PRIu64 ": ", i);

    for (u64 j = 0; j < bytes_per_line; j++) {
      if (i + j < len) {
        fprintf(ofp, "%02" PRIu8 " ", data[i + j]);
      } else {
        fprintf(ofp, "   ");
      }
    }

    printf(" |");
    for (u64 j = 0; j < bytes_per_line; j++) {
      if (i + j < len) {
        u8 ch = data[i + j];
        printf("%c", isprint(ch) ? ch : '.');
      }
    }
    printf("|\n");
  }
}
