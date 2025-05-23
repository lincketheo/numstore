#pragma once

#include "mm/lalloc.h"
#include "paging/types/inner_node.h"

/**
 * Represents an internal node in memory
 *
 * To emulate len(leafs) == len(keys) + 1, I just save first_leaf
 * and then a list of structs (less allocations)
 */
typedef struct
{
  /**
   *        keys[0]       keys[1]
   * values[0]    values[1]     values[2]
   */
  p_size keys[50];
  pgno values[51];
  u32 klen;
} mem_inner_node;

void meminode_create (mem_inner_node *dest, pgno pg);

u32 meminode_avail (mem_inner_node *r);

bool meminode_full (mem_inner_node *r);

/**
 *    1   2                  1   2   5
 * a    b    c  + (3, d) = a   b   c   d
 */
void meminode_push_right (mem_inner_node *r, b_size key, pgno pg);

/**
 *    1   2                  1   2   3
 * a    b    c  + (3, d) = a   b   c   d
 */
void meminode_push_right_no_add (mem_inner_node *r, b_size key, pgno pg);

/**
 *             2   3         1   3   4
 * (1, a) + b    c    d  = a   b   c   d
 */
void meminode_push_left (mem_inner_node *r, pgno pg, b_size key);

/**
 *             2   3         1   2   3
 * (1, a) + b    c    d  = a   b   c   d
 */
void meminode_push_left_no_add (mem_inner_node *r, pgno pg, b_size key);

/**
 *    2   5   9         3   7
 * a    b    c   d  = b   c    d
 *
 * exp = a
 * return 2
 */
typedef struct
{
  b_size key;
  pgno value;
} meminode_kv;

meminode_kv meminode_pop_left (mem_inner_node *r, pgno exp);

void meminode_write_max (inner_node *dest, mem_inner_node *m);
