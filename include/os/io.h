#pragma once

#include "common/types.h"

// An opaque file for abstraction
typedef struct xfd_s xfd;

int xfd_valid(const xfd* f);

DEFINE_ASSERT(xfd, xfd)

typedef enum {
  XFD_R,
  XFD_RW,
  XFD_W,
} xfd_opt;

xfd* xfd_open(const char* fname, ...);

int xfd_tell();

i64 xfd_size(xfd* f);

int xfd_extend(xfd* f, u32 bytes);

int stderr_out_io(const char* fmt, ...);

int stdout_out_io(const char* fmt, ...);
