#include "os.h"

#include "dev/assert.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/////////////////////// Allocation
void* rads_malloc(size_t bytes) {
  ASSERT(bytes > 0);
  void* ret = malloc((size_t)bytes);
  if (ret == NULL) {
    log_error("Failed to allocate %zu bytes. Reason: %s", bytes, strerror(errno));
  }
  return ret;
}

void rads_free(void* ptr) {
  ASSERT(ptr);
  free(ptr);
}

void* rads_realloc(void* ptr, int bytes) {
  ASSERT(ptr);
  ASSERT(bytes > 0);
  void* ret = realloc(ptr, bytes);
  if (ret == NULL) {
    log_error("Failed to reallocate %zu bytes. Reason: %s", bytes, strerror(errno));
  }
  return ret;
}

/////////////////////// Files
struct rads_file {
  int fd;
};

DEFINE_ASSERT(rads_file, rads_file, fp) {
  if(fp == NULL) {
    return 0;
  }
  return fcntl(fp->fd, F_GETFD) != -1 || errno != EBADF;
}

rads_file* rads_open(const char* fname, int read, int write) {
  ASSERT(fname);

  rads_file *ret = rads_malloc(sizeof ret);
  if (ret == NULL) {
    return ret;
  }

  if(read && write) {
    ret->fd = open(fname, O_RDWR);
  } else if(read) {
    ret->fd = open(fname, O_RDONLY);
  } else if(write) {
    ret->fd = open(fname, O_WRONLY);
  } else {
    log_error("Refusing to open file: %s without either read or write", fname);
    return NULL;
  }

  rads_file_assert(ret);

  return ret;
}

int rads_close(rads_file* fp) {
  ASSERT(fp);
}

i64 rads_read(rads_file* fp, void* dest, int n, i64 offset);

i64 rads_write(rads_file* fp, const void* src, int n, i64 offset);

/////////////////////// Logging
void log_trace(const char* fmt, ...);

void log_debug(const char* fmt, ...);

void log_info(const char* fmt, ...);

void log_warn(const char* fmt, ...);

void log_error(const char* fmt, ...);
