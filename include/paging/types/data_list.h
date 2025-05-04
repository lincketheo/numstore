#pragma once

#include "intf/types.h"

/**
 * DATA LIST
 * ============ PAGE START
 * HEADER
 * NKEYS
 * LENN (numerator)
 * LEND (denominator)
 * DATA0
 * DATA1
 * DATA2
 * ...
 * DATA(LENN / LEND)
 * 0
 * 0
 * ============ PAGE END
 */
typedef struct
{
  u8 *raw;
  p_size rlen;

  pgh *header;  // Page header
  pgno *next;   // Pointer to the next node or 0
  p_size *blen; // The length of this node in bytes
  u8 *data;     // The raw contiguous data pointer
} data_list;

bool dl_is_valid (data_list *d);
data_list dl_set_ptrs (u8 *raw, p_size len);
p_size dl_avail (data_list *d);
void dl_init_empty (data_list *d);
p_size dl_write (data_list *d, const u8 *src, p_size bytes);
p_size dl_read (data_list *d, u8 *dest, p_size offset, p_size bytes);
p_size dl_read_out (data_list *d, u8 *dest, p_size offset);
void dl_set_next (data_list *d, pgno next);
