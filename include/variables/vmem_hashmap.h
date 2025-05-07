#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "query/queries/create.h"
#include "type/types.h"
#include "variable.h"

/**
 * A variable hash map in memory
 *
 * Stores hashes to variables and their meta information
 */

typedef struct vnode_s
{
  const string key;     // The variable name
  vmeta vmeta;          // The variable value
  struct vnode_s *next; // Link to the next node (a linked list)
} vnode;

/**
 * Wrapper around linked list
 */
typedef struct
{
  vnode *head;
} helem;

typedef struct
{
  helem *elems; // The hash map
  u32 len;      // Length of the hash map

  lalloc *string_allocator; // Allocates strings for variables
  lalloc *type_allocator;   // Allocates types for variables
  lalloc *map_allocator;    // Allocates the entire hash map
  lalloc *node_allocator;   // Allocates nodes for the hash map
} vmem_hashmap;

typedef struct
{
  u32 len;

  lalloc *string_allocator;
  lalloc *type_allocator;
  lalloc *map_allocator;
  lalloc *node_allocator;
} vmhm_params;

/**
 * Creates a var retr (not to be confused with the CREATE api call)
 * Returns:
 *   - ERR_NOMEM if you don't have enough room to allocate elems
 */
err_t vmhm_create (vmem_hashmap *dest, vmhm_params params);

/**
 * Used for building variable retriever
 * Returns:
 *   - ERR_ALREADY_EXISTS if [key] exists
 *   - ERR_NOMEM if node_allocator can't create node
 *   - ERR_NOMEM if type_allocator can't create a copy of the type
 */
err_t vmhm_insert (
    vmem_hashmap *h,
    alloced_string key,
    alloced_vmeta value);

err_t vmhm_update_pg0 (
    vmem_hashmap *h,
    const string key,
    pgno pg0);

/**
 * Fetche's nodes or returns ERR_DOESNT_EXIST
 */
err_t vmhm_get (const vmem_hashmap *v, vmeta *dest, const string key);
