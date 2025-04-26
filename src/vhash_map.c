#include "vhash_map.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (vhash_map, vhash_map, v)
{
  ASSERT (v);
  ASSERT (v->elems);
}

static inline u32
fnv1a_hash (const string s)
{
  u32 hash = 2166136261u;
  char *str = s.data;
  for (u32 i = 0; i < s.len; ++i)
    {
      hash ^= (u8)*str++;
      hash *= 16777619u;
    }
  return hash;
}

err_t
vhash_map_create (vhash_map *dest, vhm_params params)
{
  ASSERT (dest);
  ASSERT (params.len > 0);

  dest->elems = lcalloc (
      params.map_allocator,
      params.len,
      sizeof *dest->elems);
  if (!dest->elems)
    {
      return ERR_NOMEM;
    }

  dest->type_allocator = params.type_allocator;
  dest->node_allocator = params.node_allocator;
  dest->len = params.len;

  return SUCCESS;
}

static inline err_t
helem_insert (
    helem *node,
    const string key,
    vmeta value,
    lalloc *node_allocator)
{
  ASSERT (node);

  vnode *curr = node->head;
  vnode *prev = NULL;

  // Traverse to the end while checking that no dups are found
  while (curr)
    {
      if (string_equal (curr->key, key))
        {
          return ERR_ALREADY_EXISTS;
        }
      prev = curr;
      curr = curr->next;
    }

  // Allocate new node
  vnode *new_node = lmalloc (node_allocator, sizeof *new_node);
  if (!new_node)
    {
      return ERR_NOMEM;
    }

  // Allocate and copy over new string
  char *newstr = lmalloc (node_allocator, key.len);
  if (!newstr)
    {
      lfree (node_allocator, new_node);
      return ERR_NOMEM;
    }
  i_memcpy (newstr, key.data, key.len);

  vnode _new_node = {
    .key = (string){
        .data = newstr,
        .len = key.len,
    },
    .vmeta = value,
    .next = NULL,
  };
  i_memcpy (new_node, &_new_node, sizeof (_new_node));

  if (prev)
    {
      prev->next = new_node;
    }
  else
    {
      node->head = new_node;
    }

  return SUCCESS;
}

err_t
vhash_map_insert (vhash_map *h, const string key, vmeta value)
{
  vhash_map_assert (h);
  ASSERT (h);

  u32 idx = fnv1a_hash (key);

  return helem_insert (&h->elems[idx], key, value, h->node_allocator);
}

static inline err_t
helem_get (vmeta *dest, const helem *node, const string v)
{
  ASSERT (node);

  vnode *curr = node->head;

  while (curr)
    {
      if (string_equal (curr->key, v))
        {
          if (dest)
            {
              i_memcpy (dest, &curr->vmeta, sizeof (curr->vmeta));
            }
          return SUCCESS;
        }
      curr = curr->next;
    }

  return ERR_DOESNT_EXIST;
}

err_t
vhash_map_get (vmeta *dest, const vhash_map *h, const string vname)
{
  ASSERT (h && h->elems);

  u32 idx = fnv1a_hash (vname);

  return helem_get (dest, &h->elems[idx], vname);
}
