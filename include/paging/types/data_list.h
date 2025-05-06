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

/**
 * Checks that this data list is valid
 */
bool dl_is_valid (const data_list *d);

/**
 * Simply parses raw and sets pointers and returns
 * the data_list pointer struct
 * Doesn't modify raw data at all or do any validity
 * checking
 */
data_list dl_set_ptrs (u8 *raw, p_size len);

/**
 * Returns the size of the data chunk of data_list node
 */
p_size dl_data_size (p_size page_size);

/**
 * Returns how many bytes are available to write to in this node
 */
p_size dl_avail (const data_list *d);

/**
 * Sets the content inside d so that it's
 * empty - used on an init call
 */
void dl_init_empty (data_list *d);

/**
 * Writes as much data from [src] as it can (up until bytes)
 * Returns the number of bytes written
 */
p_size dl_write (data_list *d, const u8 *src, p_size bytes);

/**
 * Reads as much data from [d] into [dest] as it
 * can from [offset] to [offset + bytes]
 */
p_size dl_read (const data_list *d, u8 *dest, p_size offset, p_size bytes);

/**
 * Reads out data from [offset] to the end of this page to dest
 * Returns the number of bytes read
 * This alters data_list (subtracts from dlen)
 */
p_size dl_read_out_from (data_list *d, u8 *dest, p_size offset);

/**
 * Returns how many bytes are used (nbytes)
 */
p_size dl_used (const data_list *d);

/**
 * Gets / Sets the next pointer
 */
pgno dl_get_next (const data_list *d);
void dl_set_next (data_list *d, pgno next);
