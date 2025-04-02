#pragma once

#include "types.h"

///////////////// Struct to "Allocate" and "Deallocate" pages of a file

typedef struct {
  int fd;
} page_alloc;

#define page_alloc_assert(p) \
assert(p); \
fd_assert((p)->fd);

// Lifecycle
int page_alloc_open(page_alloc* dest, const char* fname);

int page_alloc_close(page_alloc* p);

// Allocation Routines
spage_ptr page_alloc_new(page_alloc* p, u8 dest[PAGE_SIZE]);

int page_alloc_delete(page_alloc* p, u8 dest[PAGE_SIZE]);

int page_alloc_get(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr);

int page_alloc_update(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr);

