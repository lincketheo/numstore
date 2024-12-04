#pragma once

#include <stdbool.h> // bool
#include <stdlib.h>  // size_t

///////////////////////////////
////////// Quick Heap

// Returns qh "context"
size_t quick_heap_start ();

bool quick_heap_avail (size_t ctx, size_t bytes);

void *quick_heap_malloc (size_t *ctx, size_t bytes);

///////////////////////////////
////////// Linalloc

typedef struct
{
  void *data;
  const size_t cap;
  size_t len;
} linalloc;

linalloc linalloc_create_qh ();

int linalloc_avail (linalloc *l, size_t bytes);

void *linalloc_malloc (linalloc *l, size_t bytes);

void linalloc_free (linalloc *l, void *ptr);

///////////////////////////////
////////// Abstraction layer

typedef enum
{
  LINEAR,
} ns_alloc_type;

typedef struct
{
  union
  {
    linalloc l;
  };
  ns_alloc_type type;
} ns_alloc;

int ns_alloc_avail (ns_alloc *a, size_t bytes);

ns_alloc ns_alloc_create_qh (ns_alloc_type type);

void *ns_alloc_malloc (ns_alloc *a, size_t bytes);

void ns_alloc_free (ns_alloc *a, void *ptr);
