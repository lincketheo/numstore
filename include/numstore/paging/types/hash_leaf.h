#pragma once

#include "core/errors/error.h" // TODO
#include "core/intf/types.h"   // TODO

#include "numstore/config.h" // TODO

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
  pgh *header;
  pgno *next; // Pointer to next hash map leaf - 0 if none
  u8 *data;   // Pointer to serialized variables
} hash_leaf;

#define HL_HEDR_OFST ((p_size)0)
#define HL_NEXT_OFST ((p_size)(HL_HEDR_OFST + sizeof (pgh)))
#define HL_DATA_OFST ((p_size)(HL_NEXT_OFST + sizeof (pgno)))

_Static_assert(PAGE_SIZE > HL_DATA_OFST + 10,
               "Hash Leaf: PAGE_SIZE must be > HL_DATA_OFST "
               "plus at least 10 extra bytes of data");
#define HL_DATA_LEN ((p_size)(PAGE_SIZE - HL_DATA_OFST))

err_t hl_validate (const hash_leaf *p, error *e);
void hl_set_ptrs (hash_leaf *hl, u8 raw[PAGE_SIZE]);
void hl_init_empty (hash_leaf *hl);
pgno hl_get_next (const hash_leaf *hl);
void hl_set_next (hash_leaf *hl, pgno next);
void i_log_hl (const hash_leaf *hl);
