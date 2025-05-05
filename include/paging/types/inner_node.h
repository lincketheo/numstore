#pragma once

#include "dev/errors.h"
#include "intf/types.h"

/**
 * INNER NODE
 * ============ PAGE START
 * HEADER
 * NKEYS
 * LEAF0
 * LEAF1
 * ...
 * LEAF(NKEYS)
 * 0
 * 0
 * 0
 * ...
 * 0
 * 0
 * 0
 * KEY(NKEYS - 1)
 * ...
 * KEY1
 * KEY0
 * ============ PAGE END
 */
typedef struct
{
  u8 *raw;
  p_size rlen;

  pgh *header;
  p_size *nkeys; // Number of keys
  pgno *leafs;   // len(leafs) == nkeys + 1
  b_size *keys;  // The keys used for rope traversal
} inner_node;

/**
 * Examples:
 *
 * Keys = [5, 10, 15, 20, 25]
 * Leafs = [p0, p1, p2, p3, p4, p5]
 * idx = 17
 * return = p3
 * left = 15
 *
 * Keys = [5, 10, 15, 20, 25]
 * Leafs = [p0, p1, p2, p3, p4, p5]
 * idx = 0
 * return p0
 * left = 0
 *
 * Keys = [5]
 * leafs = [p0, p1]
 * idx = 17
 * return p1
 * left = 5
 */
p_size in_choose_lidx (const inner_node *node, b_size loc);
void in_add_right (inner_node *node, p_size from, b_size add);
inner_node in_set_initial_ptrs (u8 *raw, p_size len);
void in_init_empty (inner_node *in);
err_t in_read_and_set_ptrs (inner_node *dest, u8 *raw, p_size len);
void in_init (inner_node *dest, b_size key, pgno left, pgno right);
bool in_add_kv (inner_node *dest, b_size key, pgno right);
