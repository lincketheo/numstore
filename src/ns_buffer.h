#pragma once

#include "ns_bytes.h"
#include "ns_errors.h"
#include "ns_srange.h"
#include "ns_types.h"

///////////////////////////////
////////// Buffer

/**
 * A buffer is an abstraction
 * for a filled data source
 *
 * With a capacity, and len (all in
 * terms of element size)
 */
typedef struct
{
  ns_byte *data;
  ns_size size;
  ns_size cap;
  ns_size len;
} buf;

// buf assertions
#define buf_ASSERT(bptr)                                                      \
  ASSERT (bptr);                                                              \
  ASSERT ((bptr)->data);                                                      \
  ASSERT ((bptr)->size > 0);                                                  \
  ASSERT ((bptr)->cap > 0);                                                   \
  ASSERT ((bptr)->len <= (bptr)->cap)

// Constructors
#define buf_create_from(_data, _cap, _len)                                    \
  (buf)                                                                       \
  {                                                                           \
    .data = (ns_byte *)_data, .size = sizeof *_data, .cap = _cap, .len = _len \
  }

buf buf_create_empty_from_bytes (bytes b, ns_size size);

// How much room is left. 0 if full
ns_size buf_avail (buf b);

// Shift all elements from index ind onwards down to the start
// ind must be valid
buf buf_shift_mem (buf b, ns_size ind);

#define buftobytes(b) bytes_create_from ((b).data, (b).cap *(b).size)
#define buftobytesp(b) bytes_create_from ((b)->data, (b)->cap *(b)->size)

void buf_move_buf (buf *dest, buf *src, ssrange s);

void buf_append_buf (buf *dest, buf *src, ssrange s);
