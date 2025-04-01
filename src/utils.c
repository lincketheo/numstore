#include "utils.h"
#include "errors.h"
#include "ns_assert.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

///////// FILES
i64 file_size (int fd) {
  fd_assert(fd);
  off_t offset = lseek(fd, 0, SEEK_END);
  if(offset < 0) {
    perror("lseek");
    return ERR_IO;
  }
  return (i64)offset;
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
