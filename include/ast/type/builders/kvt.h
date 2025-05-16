#pragma once

#include "ast/type/types.h" // type
#include "ds/llist.h"       // llnode
#include "ds/strings.h"     // string
#include "intf/types.h"     // u32
#include "mm/lalloc.h"      // lalloc

/**
 * An intrusive linked list
 * with string/type payload
 */
typedef struct
{
  string key;
  type value;
  llnode link;
} kv_llnode;

typedef struct
{
  llnode *head;

  u16 klen;
  u16 tlen;

  lalloc *alloc; // For growing types and keys arrays
} kvt_builder;

kvt_builder kvb_create (lalloc *alloc);

/**
 * Allocates a new node if klen < len(head)
 *
 * Errors:
 *   - ERR_INVALID_ARGUMENT:
 *      - Duplicate Key
 *   - ERR_NOMEM:
 *      - No memory to allocate a new node (sizeof(kv_llnode))
 */
err_t kvb_accept_key (kvt_builder *eb, string key, error *e);

/**
 * Allocates a new node if klen < len(head)
 *
 * Errors:
 *   - ERR_NOMEM:
 *      - No memory to allocate a new node (sizeof(kv_llnode))
 * Assumes type is valid
 */
err_t kvb_accept_type (kvt_builder *eb, type t, error *e);

/**
 * Errors:
 *   - ERR_INVALID_ARGUMENT:
 *       - Key length = 0
 *       - Key length != Type length
 *   - ERR_NOMEM:
 *       - No memory to allocate onto [destination]
 *          - takes klen * sizeof(string) + tlen * sizeof(type) bytes
 */
err_t kvb_union_t_build (
    union_t *dest,       // The destination union type
    kvt_builder *eb,     // The completed builder
    lalloc *destination, // Where to allocate the destination data onto
    error *e             // Compilation errors go here
);

// Same as [kvb_union_t_build],
// but moves data into struct_t.keys and struct_t.types
err_t kvb_struct_t_build (
    struct_t *dest,
    kvt_builder *eb,
    lalloc *destination,
    error *e);
