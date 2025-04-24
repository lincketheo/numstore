#pragma once

#include "sds.h"
#include "typing.h"
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
  lalloc *alloc;
} vhash_map;

err_t vhash_map_create (vhash_map *dest, u32 len, lalloc *alloc);
err_t vhash_map_insert (vhash_map *h, const string key, vmeta value);
err_t vhash_map_get (vmeta *dest, const vhash_map *h, const string key);
