#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "query/queries/create.h"
#include "type/types.h"
#include "variable.h"

typedef struct vnode_s
{
  const string key;
  vmeta vmeta;
  struct vnode_s *next;
} vnode;

typedef struct
{
  vnode *head;
} helem;

typedef struct
{
  helem *elems;
  u32 len;

  lalloc *type_allocator;
  lalloc *node_allocator;
} vhash_map;

typedef struct
{
  u32 len;
  lalloc *type_allocator;
  lalloc *node_allocator;
  lalloc *map_allocator;
} vhm_params;

/**
 * Creates a var retr (not to be confused with the CREATE api call)
 * Returns:
 *   - ERR_NOMEM if you don't have enough room to allocate elems
 */
err_t vhash_map_create (vhash_map *dest, vhm_params params);

/**
 * Used for building variable retriever
 * Returns:
 *   - ERR_ALREADY_EXISTS if [key] exists
 *   - ERR_NOMEM if node_allocator can't create node
 *   - ERR_NOMEM if type_allocator can't create
 */
err_t vhash_map_insert (vhash_map *h, const string key, vmeta value);
err_t vhash_map_get (const vhash_map *v, vmeta *dest, const string key);
