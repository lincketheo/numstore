#pragma once

#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"

typedef struct page_s page;

/**
 * HASH LEAF
 * ============ PAGE START
 * HEADER
 * NEXT
 * VLEN         (2 bytes)
 * ISPRESENT    (0 / 1 - 1 byte)
 * PG0          (pgno)
 * VSTRLEN      (2 bytes)
 * TSTRLEN
 * VNAME
 * VNAME
 * VNAME
 * ...
 * TSTR
 * TSTR
 * ...
 * VLEN
 * ....
 * ============ PAGE END
 */
typedef struct
{
  u8 *raw;
  p_size rlen;

  pgh *header;
  pgno *next; // Pointer to next hash map leaf - 0 if none
  u8 *data;   // Pointer to serialized variables
} hash_leaf;

bool hl_is_valid (const hash_leaf *d);

p_size hl_data_len (p_size page_size);

void hl_init_empty (hash_leaf *hl);

hash_leaf hl_set_ptrs (u8 *raw, p_size len);

pgno hl_get_next (const hash_leaf *hl);

void hl_set_next (hash_leaf *hl, pgno next);
