#include "ast/type/builders/enum.h"

#include "dev/assert.h" // DEFINE_DBG_ASSERT_I
#include "errors/error.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (enum_builder, enum_builder, s)
{
  ASSERT (s);
}

static const char *TAG = "Enum Builder";

enum_builder
enb_create (
    lalloc *alloc,
    lalloc *dest)
{
  enum_builder builder = {
    .head = NULL,
    .alloc = alloc,
    .dest = dest,
  };
  return builder;
}

static bool
enb_has_key_been_used (const enum_builder *eb, string key)
{
  for (llnode *it = eb->head; it; it = it->next)
    {
      k_llnode *kn = container_of (it, k_llnode, link);
      if (string_equal (kn->key, key))
        {
          return true;
        }
    }
  return false;
}

err_t
enb_accept_key (enum_builder *eb, const string key, error *e)
{
  enum_builder_assert (eb);

  if (key.len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Key length must be > 0", TAG);
    }
  if (enb_has_key_been_used (eb, key))
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Key '%.*s' already used",
          TAG, key.len, key.data);
    }

  u16 idx = (u16)list_length (eb->head);
  llnode *slot = llnode_get_n (eb->head, idx);
  k_llnode *node;
  if (slot)
    {
      node = container_of (slot, k_llnode, link);
    }
  else
    {
      node = lmalloc (eb->alloc, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: allocation failed", TAG);
        }
      llnode_init (&node->link);
      node->key = (string){ 0 };
      if (!eb->head)
        {
          eb->head = &node->link;
        }
      else
        {
          list_append (&eb->head, &node->link);
        }
    }

  char *dest = lmalloc (eb->dest, key.len, 1);
  if (dest == NULL)
    {
      return error_causef (e, ERR_NOMEM, "%s Failed to copy enum string", TAG);
    }
  i_memcpy (dest, key.data, key.len);

  node->key = (string){
    .data = dest,
    .len = key.len,
  };
  return SUCCESS;
}

err_t
enb_build (
    enum_t *dest,
    enum_builder *eb,
    error *e)
{
  enum_builder_assert (eb);
  ASSERT (dest);

  u16 len = (u16)list_length (eb->head);
  if (len == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: no keys to build", TAG);
    }

  string *keys = lmalloc (eb->dest, len, sizeof *keys);
  if (!keys)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: failed to allocate keys array", TAG);
    }

  u16 i = 0;
  for (llnode *it = eb->head; it; it = it->next)
    {
      k_llnode *kn = container_of (it, k_llnode, link);
      keys[i++] = kn->key;
    }

  dest->len = len;
  dest->keys = keys;
  return SUCCESS;
}
