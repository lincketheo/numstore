#pragma once

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
pgno in_choose_leaf (const inner_node *node, b_size *left, b_size loc);
