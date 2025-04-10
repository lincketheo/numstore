#include "memory/page_alloc.h"
#include "errors.h"
#include "ns_assert.h"
#include "types.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

// Allocation Routines
int page_alloc_new(page_alloc* p, page* dest)
{
  page_alloc_assert(p);
  assert(dest);

  off_t pos = lseek(p->fd, 0, SEEK_END);

  if (pos < 0) {
    perror("lseek");
    return ERR_IO;
  }

  if (pos % PAGE_SIZE != 0) {
    return ERR_INVALID_STATE;
  }

  return pos / PAGE_SIZE;
}

int page_alloc_delete(page_alloc* p, u8 dest[PAGE_SIZE])
{
  page_alloc_assert(p);
  assert(dest);
  // TODO
  return SUCCESS;
}

int page_alloc_get(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr)
{
  page_alloc_assert(p);
  assert(dest);

  ssize_t read = pread(p->fd, dest, PAGE_SIZE, PAGE_SIZE * ptr);
  if (read < 0) {
    perror("pread");
    return ERR_IO;
  }

  todo_assert(read == PAGE_SIZE);

  return SUCCESS;
}

int page_alloc_update(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr)
{
  page_alloc_assert(p);
  assert(dest);

  ssize_t written = pwrite(p->fd, dest, PAGE_SIZE, PAGE_SIZE * ptr);
  if (written < 0) {
    perror("pwrite");
    return ERR_IO;
  }

  todo_assert(written == PAGE_SIZE);

  return SUCCESS;
}
