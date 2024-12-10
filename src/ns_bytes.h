#pragma once

#include "ns_errors.h"
#include "ns_types.h"
#include "ns_errors.h"

///////////////////////////////
////////// bytes

/**
 * All allocations return bytes,
 * so there is no such thing as
 * dangling pointers without
 * memory capacity
 */
typedef struct
{
  ns_byte *data;
  ns_size cap;
} bytes;

// bytes assertions for valid states
#define bytes_IS_FREE(b) ((b)->data == NULL && (b)->cap == 0)
#define bytes_ASSERT(b)                                                       \
  ASSERT (b);                                                                 \
  ASSERT (bytes_IS_FREE (b) || !bytes_IS_FREE (b))

////////// Constructors
#define bytes_create_from(d, c)                                               \
  (bytes) { .data = d, .cap = c, }

#define bytes_create_empty() bytes_create_from (NULL, 0)

#define bytes_update(p, d, c)                                                 \
  (bytes) { .data = d, .cap = c, }

////////// Utilities
void ns_memcpy (bytes *dest, const bytes *src, ns_size size);

void ns_memmove (bytes *dest, const bytes *src, ns_size size);

bytes bytes_from (const bytes src, ns_size from);
