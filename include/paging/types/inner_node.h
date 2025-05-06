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
 * Checks that this is a valid node
 */
bool in_is_valid (const inner_node *in);

/**
 *   1  5   10
 * a  b   c   d
 *
 * loc = 0 -> return 0
 * loc = 1 -> return 1
 * loc = 2 -> return 1
 * loc = 7 -> return 2
 * loc = 11 -> return 3
 */
p_size in_choose_lidx (const inner_node *node, b_size loc);

/**
 *    1    2                   1   5
 * a    b     c   + (1, 3) = a   b   c
 */
void in_add_right (inner_node *node, p_size from, b_size add);

/**
 * This sets pointers to default values
 * WARNING - it does NOT set keys to the value read in by raw
 * This doesn't interpret anything inside of raw
 *
 * After calling this, in_is_valid would be false because it's meaningless
 */
inner_node in_set_initial_ptrs (u8 *raw, p_size len);

/**
 * Actually sets data as if this node were empty
 *
 * After calling this, in_is_valid would be false because it sets 0 keys
 */
void in_init_empty (inner_node *in);

/**
 * Same as set ptrs, but this time it reads the value at blen
 * and sets pointers. Returns ERR_INVALID_STATE if the data is invalid
 *
 * On Success, After calling this, in_is_valid would be true
 */
err_t in_read_and_set_ptrs (inner_node *dest, u8 *raw, p_size len);

/**
 * Sets klen = 1 and pushes left / right to leafs and key to keys
 *
 * After calling this, in_is_valid would be true
 */
void in_init (inner_node *dest, b_size key, pgno left, pgno right);

/**
 * Pushes a key and leaf to the right side of the node
 */
bool in_add_kv (inner_node *dest, b_size key, pgno right);

/**
 * Returns the maximum number of keys that a inner node can hold
 *
 * node must be in a valid state
 */
p_size in_max_nkeys (p_size page_size);

/**
 *    1    2     3
 * a    b     c     d   fromkey = 1
 *
 * =>
 *
 *    1
 * a    b
 */
void in_clip_right (inner_node *in, p_size fromkey);

/**
 * Returns how many keys are available
 *
 * node must be in a valid state
 */
p_size in_keys_avail (inner_node *in);

/**
 * Get number of keys
 */
p_size in_get_nkeys (const inner_node *in);

/**
 * Gets key at idx
 *
 * node must be in a valid state
 * idx must be a valid index
 */
b_size in_get_key (inner_node *in, p_size idx);

/**
 * Gets key at idx
 *
 * node must be in a valid state
 * idx must be a valid index
 */
pgno in_get_leaf (inner_node *in, p_size idx);

/**
 * Get's the right most leaf
 */
pgno in_get_right_most_leaf (inner_node *in);

/**
 * Get's the right most key
 */
b_size in_get_right_most_key (inner_node *in);
