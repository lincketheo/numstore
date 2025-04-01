#include "page_alloc.h"
#include "types.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int page_alloc_init(page_alloc* dest, const char* fname) {
  assert(dest);
  assert(fname);

  int fd = open(fname, O_CREAT | O_RDWR | O_TRUNC , S_IRUSR | S_IWUSR);
  if(fd == -1) {
    perror("open");
    return ERR_IO;
  }

  dest->fd = fd;
  return SUCCESS;
}

page_ptr page_alloc_new(page_alloc* p, u8 dest[PAGE_SIZE]) {
  off_t pos = lseek(p->fd, 0, SEEK_END);
  assert(pos % PAGE_SIZE == 0); // TODO 
  write(p->fd, 
}

void page_alloc_delete(page_alloc* p, u8 dest[PAGE_SIZE]) {
  return; // TODO
}

void page_alloc_get(page_alloc* p, u8 dest[PAGE_SIZE], page_ptr ptr) {
  ssize_t r = pread(p->fd, dest, PAGE_SIZE, PAGE_SIZE * ptr);
  assert(r == PAGE_SIZE); // TODO
}

page_ptr page_alloc_new(u8 dest[PAGE_SIZE]) {

}

page_ptr page_alloc_delete(u8 dest[PAGE_SIZE]) {

}

void page_alloc_get(u8 dest[PAGE_SIZE], page_ptr ptr) {

}
