#pragma once

#include "config.h"       // PAGE_SIZE
#include "errors/error.h" // err_t
#include "intf/types.h"   // u8

/**
 * DATA LIST
 * ============ PAGE START
 * HEADER
 * NEXT_PAGE
 * BYTE_LEN
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
  pgh *header;  // Page header
  pgno *next;   // Pointer to the next node or 0
  p_size *blen; // The length of this node in bytes
  u8 *data;     // The raw contiguous data pointer
} data_list;

#define DL_HEDR_OFST ((p_size)0)
#define DL_NEXT_OFST ((p_size)(DL_HEDR_OFST + sizeof (pgh)))
#define DL_BLEN_OFST ((p_size)(DL_NEXT_OFST + sizeof (pgno)))
#define DL_DATA_OFST ((p_size)(DL_BLEN_OFST + sizeof (p_size)))

_Static_assert (PAGE_SIZE > DL_DATA_OFST + 10,
                "Data List: PAGE_SIZE must be > DL_DATA_OFST "
                "plus at least 10 extra bytes of data");
#define DL_DATA_SIZE ((p_size)(PAGE_SIZE - DL_DATA_OFST))

err_t dl_validate (const data_list *d, error *e);
void dl_set_ptrs (data_list *dl, u8 raw[PAGE_SIZE]);
p_size dl_avail (const data_list *d);
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

void i_log_dl (const data_list *d);
