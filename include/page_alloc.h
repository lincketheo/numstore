#pragma once

#include "types.h"
#include "errors.h"

///////////////// Mean to "Allocate" and "Deallocate" pages of a file

typedef struct {
  int fd;
} page_alloc;

err_t page_alloc_open(page_alloc* dest, const char* fname);

err_t page_alloc_close(page_alloc* p);

page_ptr page_alloc_new(page_alloc* p, u8 dest[PAGE_SIZE]);

void page_alloc_delete(page_alloc* p, u8 dest[PAGE_SIZE]);

void page_alloc_get(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr);

void page_alloc_save(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr);
