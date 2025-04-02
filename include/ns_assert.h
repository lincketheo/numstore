#pragma once

#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#define todo_assert(expr) \
  assert(expr)

static inline int _fd_assert(int fd) {
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

#define fd_assert(fd) _fd_assert(fd)
