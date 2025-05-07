#include "variables/vmem_hashmap.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "query/queries/create.h"
#include "variables/variable.h"

DEFINE_DBG_ASSERT_I (vmem_hashmap, vmem_hashmap, v)
{
  ASSERT (v);
  ASSERT (v->elems);
}

err_t
vmhm_create (vmem_hashmap *dest, vmhm_params params)
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

  dest->len = params.len;
  dest->string_allocator = params.string_allocator;
  dest->type_allocator = params.type_allocator;
  dest->map_allocator = params.map_allocator;
  dest->node_allocator = params.node_allocator;

  return SUCCESS;
}

vnode *
vnode_alloc_create (
    vmem_hashmap *h,
    alloced_string key,
    alloced_vmeta vmeta)
{
  err_t ret;

  /**
   * Allocate new node
   */
  vnode *new_node = lmalloc (h->node_allocator, sizeof *new_node);
  if (!new_node)
    {
      return NULL;
    }

  /**
   * Transfer ownership of
   * this pointer
   */
  char *new_string = lalloc_xfer (
      h->string_allocator,
      key.alloc,
      key.str.data);

  if (!new_string)
    {
      lfree (h->node_allocator, new_node);
      return NULL;
    }

  string newkey = {
    .data = new_string,
    .len = key.str.len,
  };

  vmeta new_value;

  vnode _new_node = {
    .key = newkey,
    .vmeta = value,
    .next = NULL,
  };

  i_memcpy (new_node, &_new_node, sizeof (_new_node));

  return new_node;
}

err_t
vmhm_insert (
    vmem_hashmap *h,
    const string key,  // The variable name
    lalloc *key_alloc, // The allocator we used to allocate the key
    vmeta value,       // The variable meta information
    lalloc *type_alloc // The allocator used to allocate the type
)
{
  vmem_hashmap_assert (h);

  // Location of the new key
  u32 idx = fnv1a_hash (key) % h->len;

  // Root node
  helem *root = &h->elems[idx];

  /**
   * Create the new node
   */
  vnode *new_node = vnode_alloc_create (key, value, h->string_allocator);
  if (new_node == NULL)
    {
      return ERR_NOMEM;
    }

  /**
   * Traverse the linked list to insert it
   */
  {
    vnode *curr = root->head;
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

    if (prev)
      {
        prev->next = new_node;
      }
    else
      {
        root->head = new_node;
      }
  }

  return SUCCESS;
}

err_t
vmhm_update_pg0 (vmem_hashmap *h, const string key, pgno pg0)
{
  vmem_hashmap_assert (h);

  u32 idx = fnv1a_hash (key) % h->len;
  helem *node = &h->elems[idx];

  vnode *new_node = NULL;

  /**
   * Create the new node
   */
  {
    new_node = lmalloc (h->node_allocator, sizeof *new_node);
    if (!new_node)
      {
        return ERR_NOMEM;
      }

    // Allocate and copy over new string
    char *newstr = lmalloc (h->node_allocator, key.len);
    if (!newstr)
      {
        lfree (h->node_allocator, new_node);
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
  }

  /**
   * Traverse the linked list to insert it
   */
  {
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

    if (prev)
      {
        prev->next = new_node;
      }
    else
      {
        node->head = new_node;
      }
  }

  return SUCCESS;
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
vmhm_get (const vmem_hashmap *h, vmeta *dest, const string vname)
{
  ASSERT (h && h->elems);

  u32 idx = fnv1a_hash (vname) % h->len;

  return helem_get (dest, &h->elems[idx], vname);
}

err_t
vmhm_create_var (vmem_hashmap *v, create_query cargs)
{
  vmem_hashmap_assert (v);

  err_t ret = SUCCESS;

  vmeta meta = {
    .type = cargs.type,
    .pgn0 = 0, // TODO
  };

  if ((ret = vmhm_insert (v, cargs.vname, meta)))
    {
      return ret;
    }

  return ret;
}
