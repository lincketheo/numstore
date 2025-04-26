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
  u16 *nkeys; // Number of keys
  u64 *leafs; // len(leafs) == nkeys + 1
  u64 *keys;  // The keys used for rope traversal
} inner_node;
