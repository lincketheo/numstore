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

u64 *in_get_keys_ptr (const u8 *raw, u32 page_size);

/**
 * Picks which child contains [loc]
 * Sets before to the number to the left
 * of the selected key so that you can
 * subtract loc - *before to get the next
 * loc for the layer beneath
 */
u64 in_choose_leaf (const inner_node *node, u64 *before, u64 loc);

u64 in_get_nkeys (const inner_node *node);
u64 in_get_key (const inner_node *node, u16 idx);
u64 in_get_leaf (const inner_node *node, u16 idx);
