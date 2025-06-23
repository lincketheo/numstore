#pragma once

#include "core/errors/error.h" // err_t
#include "core/intf/types.h"   // b_size
#include "numstore/config.h"   // PAGE_SIZE

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
  pgh *header;   // Page header (PG_INNER_NODE)
  p_size *nkeys; // Number of keys
  pgno *leafs;   // len(leafs) == nkeys + 1
  b_size *keys;  // The keys used for rope traversal
} inner_node;

#define IN_HEDR_OFST ((p_size)0)
#define IN_NKEY_OFST ((p_size)(IN_HEDR_OFST + sizeof (pgh)))
#define IN_LEAF_OFST ((p_size)(IN_NKEY_OFST + sizeof (p_size)))

/**
 * Derivation:
 *
 * nbytes = PAGE_SIZE - IN_LEAF_OFST
 * N * sizeof(key) + (N + 1) * sizeof(pgno) <= nbytes
 * N * (sizeof(key) + sizeof(pgno)) + sizeof(pgno) <= nbytes
 * N <= (nbytes - sizeof(value))/(sizeof(key) + sizeof(value))
 */
_Static_assert(PAGE_SIZE > IN_LEAF_OFST + 5 * sizeof (b_size) + 6 * sizeof (pgno),
               "Inner Node: PAGE_SIZE must be > IN_LEAF_OFST plus at least 5 keys");
#define IN_MAX_KEYS ((p_size)(((PAGE_SIZE - IN_LEAF_OFST) - sizeof (pgno)) \
                              / (sizeof (pgno) + sizeof (p_size))))
_Static_assert(IN_MAX_KEYS > 5, "Inner Node: IN_MAX_KEYS must be > 5");

err_t in_validate (const inner_node *in, error *e);

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
void in_set_initial_ptrs (inner_node *in, u8 raw[PAGE_SIZE]);

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
err_t in_read_and_set_ptrs (inner_node *dest, u8 raw[PAGE_SIZE], error *e);

/**
 * Sets klen = 1 and pushes left / right to leafs and key to keys
 *
 * After calling this, in_is_valid would be true
 */
void in_init (
    inner_node *dest,
    b_size key,
    pgno left,
    pgno right);

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
p_size in_keys_avail (const inner_node *in);

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
b_size in_get_key (const inner_node *in, p_size idx);

/**
 * Gets key at idx
 *
 * node must be in a valid state
 * idx must be a valid index
 */
pgno in_get_leaf (const inner_node *in, p_size idx);

/**
 * Get's the right most leaf
 */
pgno in_get_right_most_leaf (const inner_node *in);

/**
 * Get's the right most key
 */
b_size in_get_right_most_key (const inner_node *in);

/**
 * Log an inner node
 */
void i_log_in (const inner_node *in);
