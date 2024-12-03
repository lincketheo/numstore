#pragma once

#include <stdlib.h>

typedef struct
{
  size_t cap;
  size_t len;
  void *data;
} linmem;

typedef struct
{
  enum
  {
    LMC_SUCCESS,
    LMC_MALLOC_FAILED,
  } type;
  int errno_if_malloc_failed;
} lmcreate_ret;

// Calls malloc
lmcreate_ret linmem_create (linmem *dest, size_t cap);

size_t linmem_start_scope (linmem *m);
void linmem_reset (linmem *m, size_t scope);

// Returns a new block of memory
// Only returns NULL when no memory left
void *linmem_malloc (linmem *m, size_t len);

// Finish linmem
void linmem_free (linmem *m);
