#pragma once

#include "ns_bytes.h"
#include "ns_errors.h"
#include "ns_types.h"

///////////////////////////////
////////// Linalloc

/**
 * Linear allocator that has memory
 * stuck in place - no way to "grow" 
 * with overflow
 */
typedef struct
{
  ns_byte *pool;
  ns_size len;
  const ns_size cap;
} limited_linalloc;

///////////////////////////////
////////// Abstraction layer

typedef struct ns_alloc_s ns_alloc;

struct ns_alloc_s
{
  union
  {
    limited_linalloc llalloc;
  };
  ns_ret_t (*ns_malloc) (ns_alloc *a, bytes *dest, ns_size cap);
  ns_ret_t (*ns_free) (ns_alloc *a, bytes *p);
  ns_ret_t (*ns_realloc) (ns_alloc *a, bytes *p, ns_size newlen);
};

ns_ret_t limited_linalloc_create (ns_alloc *dest, ns_size cap);

ns_ret_t limited_linalloc_create_from (ns_alloc *dest, ns_byte *pool,
                                       ns_size cap);

ns_ret_t stdalloc_create (ns_alloc *dest);
