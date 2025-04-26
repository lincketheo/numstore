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
  i64 *next;      // Pointer to the next node or -1
  u16 *len_num;   // Numerator of the length of this node
  u16 *len_denom; // Denominator of the length of this node
  u8 *data;       // The raw contiguous data pointer
} data_list;
