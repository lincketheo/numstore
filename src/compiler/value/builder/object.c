#include "compiler/value/builders/object.h"
#include "compiler/value/values/array.h"
#include "core/dev/assert.h"
#include "core/dev/testing.h"
#include "core/ds/llist.h"
#include "core/errors/error.h"

DEFINE_DBG_ASSERT_I (object_builder, object_builder, a)
{
  ASSERT (a);
}

static const char *TAG = "Object Value Builder";

object_builder
objb_create (lalloc *alloc, lalloc *dest)
{
  object_builder builder = {
    .head = NULL,
    .klen = 0,
    .tlen = 0,
    .work = alloc,
    .dest = dest,
  };
  object_builder_assert (&builder);
  return builder;
}

static bool
object_has_key_been_used (const object_builder *ub, string key)
{
  for (llnode *it = ub->head; it; it = it->next)
    {
      object_llnode *kn = container_of (it, object_llnode, link);
      if (string_equal (kn->key, key))
        {
          return true;
        }
    }
  return false;
}

err_t
objb_accept_string (object_builder *ub, const string key, error *e)
{
  object_builder_assert (ub);

  // Check for duplicate keys
  if (object_has_key_been_used (ub, key))
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Key: %.*s has already been used",
          TAG, key.len, key.data);
    }

  // Find where to insert this new key in the linked list
  llnode *slot = llnode_get_n (ub->head, ub->klen);
  object_llnode *node;
  if (slot)
    {
      node = container_of (slot, object_llnode, link);
    }
  else
    {
      // Allocate new node onto allocator
      node = lmalloc (ub->work, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: Failed to allocate key-value node", TAG);
        }
      llnode_init (&node->link);
      node->v = (value){ 0 };

      // Set the head if it doesn't exist
      if (!ub->head)
        {
          ub->head = &node->link;
        }

      // Otherwise, append to the list
      else
        {
          list_append (&ub->head, &node->link);
        }
    }

  // Create the node
  node->key = key;
  ub->klen++;

  return SUCCESS;
}

err_t
objb_accept_value (object_builder *ub, value t, error *e)
{
  object_builder_assert (ub);

  llnode *slot = llnode_get_n (ub->head, ub->tlen);
  object_llnode *node;
  if (slot)
    {
      node = container_of (slot, object_llnode, link);
    }
  else
    {
      node = lmalloc (ub->work, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: Failed to allocate key-value node", TAG);
        }
      llnode_init (&node->link);
      node->key = (string){ 0 };
      if (!ub->head)
        {
          ub->head = &node->link;
        }
      else
        {
          list_append (&ub->head, &node->link);
        }
    }

  node->v = t;
  ub->tlen++;
  return SUCCESS;
}

static err_t
objb_build_common (
    string **out_keys,
    value **out_types,
    u16 *out_len,
    object_builder *ub,
    lalloc *onto,
    error *e)
{
  if (ub->klen == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Expecting at least one key", TAG);
    }
  if (ub->klen != ub->tlen)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Must have same number of keys and values", TAG);
    }

  *out_keys = lmalloc (onto, ub->klen, sizeof **out_keys);
  *out_types = lmalloc (onto, ub->tlen, sizeof **out_types);
  if (!*out_keys || !*out_types)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate output arrays", TAG);
    }

  size_t i = 0;
  for (llnode *it = ub->head; it; it = it->next)
    {
      object_llnode *kn = container_of (it, object_llnode, link);
      (*out_keys)[i] = kn->key;
      (*out_types)[i] = kn->v;
      i++;
    }
  *out_len = ub->klen;
  return SUCCESS;
}

err_t
objb_build (object *dest, object_builder *ub, error *e)
{
  string *keys = NULL;
  value *values = NULL;
  u16 len = 0;

  err_t_wrap (objb_build_common (&keys, &values, &len, ub, ub->dest, e), e);

  ASSERT (keys);
  ASSERT (values);

  dest->keys = keys;
  dest->values = values;
  dest->len = len;
  return SUCCESS;
}
