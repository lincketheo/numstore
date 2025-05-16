#include "ast/type/builders/kvt.h"

#include "ast/type/types.h"
#include "dev/assert.h"

DEFINE_DBG_ASSERT_I (kvt_builder, kvt_builder, s)
{
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
}

static const char *TAG = "Key Value Type Builder";

kvt_builder
kvb_create (lalloc *alloc)
{
  kvt_builder builder = {
    .head = NULL,
    .klen = 0,
    .tlen = 0,
    .alloc = alloc,
  };
  kvt_builder_assert (&builder);
  return builder;
}

static bool
kvt_has_key_been_used (const kvt_builder *ub, string key)
{
  for (llnode *it = ub->head; it; it = it->next)
    {
      kv_llnode *kn = container_of (it, kv_llnode, link);
      if (string_equal (kn->key, key))
        {
          return true;
        }
    }
  return false;
}

err_t
kvb_accept_key (kvt_builder *ub, string key, error *e)
{
  kvt_builder_assert (ub);

  if (kvt_has_key_been_used (ub, key))
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Key: %.*s has already been used",
          TAG, key.len, key.data);
    }

  llnode *slot = llnode_get_n (ub->head, ub->klen);
  kv_llnode *node;
  if (slot)
    {
      node = container_of (slot, kv_llnode, link);
    }
  else
    {
      node = lmalloc (ub->alloc, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: Failed to allocate key-value node", TAG);
        }
      llnode_init (&node->link);
      node->value = (type){ 0 };
      if (!ub->head)
        {
          ub->head = &node->link;
        }
      else
        {
          list_append (&ub->head, &node->link);
        }
    }

  node->key = key;
  ub->klen++;
  return SUCCESS;
}

err_t
kvb_accept_type (kvt_builder *ub, type t, error *e)
{
  kvt_builder_assert (ub);

  llnode *slot = llnode_get_n (ub->head, ub->tlen);
  kv_llnode *node;
  if (slot)
    {
      node = container_of (slot, kv_llnode, link);
    }
  else
    {
      node = lmalloc (ub->alloc, 1, sizeof *node);
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

  node->value = t;
  ub->tlen++;
  return SUCCESS;
}

static err_t
kvb_build_common (
    string **out_keys,
    type **out_types,
    u16 *out_len,
    kvt_builder *ub,
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
      kv_llnode *kn = container_of (it, kv_llnode, link);
      (*out_keys)[i] = kn->key;
      (*out_types)[i] = kn->value;
      i++;
    }
  *out_len = ub->klen;
  return SUCCESS;
}

err_t
kvb_union_t_build (union_t *dest, kvt_builder *ub, lalloc *onto, error *e)
{
  string *keys;
  type *types;
  u16 len;

  err_t_wrap (kvb_build_common (&keys, &types, &len, ub, onto, e), e);

  dest->keys = keys;
  dest->types = types;
  dest->len = len;
  return SUCCESS;
}

err_t
kvb_struct_t_build (struct_t *dest, kvt_builder *ub, lalloc *onto, error *e)
{
  string *keys;
  type *types;
  u16 len;

  err_t_wrap (kvb_build_common (&keys, &types, &len, ub, onto, e), e);

  dest->keys = keys;
  dest->types = types;
  dest->len = len;
  return SUCCESS;
}
