#pragma once

#include "mm/lalloc.h"
#include "paging/types/inner_node.h"

typedef struct
{
  pgno pg;
  b_size key;
} pg_rk; // Page and right key

/**
 * Represents an internal node in memory with infinite (limited
 * by memory) keys / values
 *
 * To emulate len(leafs) == len(keys) + 1, I just save first_leaf
 * and then a list of structs (less allocations)
 */
typedef struct
{
  pg_rk *kvs;
  p_size kvlen;  // Length of kvs
  p_size kvcap;  // Capacity of kvs
  pgno first_pg; // The far left page
  lalloc *alloc; // The allocator for doubling arrays when needed
} mem_inner_node;

typedef struct
{
  lalloc *alloc;
  pgno pg;
} mintn_params;

/**
 * Lifecycle
 */
mem_inner_node mintn_create (mintn_params params);

err_t mintn_clip (mem_inner_node *r, error *e);

void mintn_free (mem_inner_node *r);

/**
 * API methods
 */

/**
 *    1   2                  1   2   5
 * a    b    c  + (3, d) = a   b   c   d
 */
err_t mintn_add_right (mem_inner_node *r, b_size key, pgno pg, error *e);

/**
 *    1   2                  1   2   3
 * a    b    c  + (3, d) = a   b   c   d
 */
err_t mintn_add_right_no_add (mem_inner_node *r, b_size key, pgno pg, error *e);

/**
 *             2   3         1   2   4
 * (1, a) + b    c    d  = a   b   c   d
 */
err_t mintn_add_left (mem_inner_node *r, pgno pg, b_size key, error *e);

/**
 *    2   5   9         3   7
 * a    b    c   d  = b   c    d
 *
 * exp = a
 * return 2
 */
b_size mintn_get_left (mem_inner_node *r, pgno exp);

void mintn_write_max_into_in (inner_node *dest, mem_inner_node *m);
