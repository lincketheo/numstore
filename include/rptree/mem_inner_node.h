#pragma once

#include "mm/lalloc.h"
#include "paging/types/inner_node.h"

typedef struct
{
  pgno pg;
  b_size key;
} pg_rk; // Page and right key

/**
 * Represents an internal node in memory
 *
 * To emulate len(leafs) == len(keys) + 1, I just save first_leaf
 * and then a list of structs (less allocations)
 */
typedef struct
{
  pg_rk kvs[50];
  p_size kvlen;  // Length of kvs
  pgno first_pg; // The far left page
} mem_inner_node;

/**
 * Lifecycle
 */
mem_inner_node mintn_create (pgno pg);

/**
 * API methods
 */

/**
 *    1   2                  1   2   5
 * a    b    c  + (3, d) = a   b   c   d
 *
 * Returns true if there was room false else
 */
bool mintn_add_right (mem_inner_node *r, b_size key, pgno pg);

/**
 *    1   2                  1   2   3
 * a    b    c  + (3, d) = a   b   c   d
 */
bool mintn_add_right_no_add (mem_inner_node *r, b_size key, pgno pg);

/**
 *             2   3         1   2   4
 * (1, a) + b    c    d  = a   b   c   d
 */
bool mintn_add_left (mem_inner_node *r, pgno pg, b_size key);

/**
 *    2   5   9         3   7
 * a    b    c   d  = b   c    d
 *
 * exp = a
 * return 2
 */
b_size mintn_get_left (mem_inner_node *r, pgno exp);

void mintn_write_max_into_in (inner_node *dest, mem_inner_node *m);
