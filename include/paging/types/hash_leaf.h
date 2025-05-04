#pragma once

#include "dev/errors.h"
#include "intf/types.h"

typedef struct page_s page;

/**
 * HASH LEAF TUPLE (inside hash leaf)
 * ============ DATA START
 * STRLEN
 * STR0
 * STR1
 * ...
 * STR(STRLEN - 1)
 * PGNO
 * TSTRLEN
 * TSTR0
 * TSTR1
 * ...
 * TSTR(TSTRLEN)
 * ============ DATA END
 */
typedef struct
{
  p_size *strlen;  // The length of the variable name
  char *str;       // The name of the variable
  pgno *pg0;       // Page 0 of the variable
  p_size *tstrlen; // The length of the type string
  char *tstr;      // The type string
} hash_leaf_tuple;

/**
 * HASH LEAF
 * ============ PAGE START
 * HEADER
 * NEXT
 * NVALUES
 * OFFSET0
 * OFFSET1
 * ...
 * OFFSET(NVALUES - 1)
 * 0
 * 0
 * ...
 * 0
 * 0
 * TPL2
 * TPL1
 * TPL0
 * ============ PAGE END
 */
typedef struct
{
  pgno *next;      // Pointer to next hash map leaf - 0 if none
  p_size *nvalues; // Number of values in this node
  p_size *offsets; // Pointers to where each key value header_sizes
} hash_leaf;

/**
 * returns:
 *  ERR_INVALID_STATE
 */
err_t hl_get_tuple (hash_leaf_tuple *dest, const page *hl, p_size idx);
