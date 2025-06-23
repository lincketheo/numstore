#include "compiler/ast/type/builders/kvt.h"

#include "compiler/ast/type/types.h" // u32
#include "core/dev/assert.h"         // ASSERT
#include "core/dev/testing.h"        // TEST
#include "core/errors/error.h"       // err_t

DEFINE_DBG_ASSERT_I (kvt_builder, kvt_builder, s)
{
  ASSERT (s);
  ASSERT (s->klen <= 10);
  ASSERT (s->tlen <= 10);
}

static const char *TAG = "Key Value Type Builder";

kvt_builder
kvb_create (lalloc *alloc, lalloc *dest)
{
  kvt_builder builder = {
    .head = NULL,
    .klen = 0,
    .tlen = 0,
    .alloc = alloc,
    .dest = dest,
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
kvb_accept_key (kvt_builder *ub, const string key, error *e)
{
  kvt_builder_assert (ub);
  i_log_info ("%s Accepting key: %.*s\n", TAG, (u32)key.len, key.data);

  // Check for duplicate keys
  if (kvt_has_key_been_used (ub, key))
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Key: %.*s has already been used",
          TAG, key.len, key.data);
    }

  // Find where to insert this new key in the linked list
  llnode *slot = llnode_get_n (ub->head, ub->klen);
  kv_llnode *node;
  if (slot)
    {
      node = container_of (slot, kv_llnode, link);
    }
  else
    {
      // Allocate new node onto allocator
      node = lmalloc (ub->alloc, 1, sizeof *node);
      if (!node)
        {
          return error_causef (
              e, ERR_NOMEM,
              "%s: Failed to allocate key-value node", TAG);
        }
      llnode_init (&node->link);
      node->value = (type){ 0 };

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
kvb_union_t_build (union_t *dest, kvt_builder *ub, error *e)
{
  string *keys = NULL;
  type *types = NULL;
  u16 len = 0;

  err_t_wrap (kvb_build_common (&keys, &types, &len, ub, ub->dest, e), e);

  ASSERT (keys);
  ASSERT (types);
  ASSERT (len > 1);

  dest->keys = keys;
  dest->types = types;
  dest->len = len;
  return SUCCESS;
}

err_t
kvb_struct_t_build (struct_t *dest, kvt_builder *ub, error *e)
{
  string *keys = NULL;
  type *types = NULL;
  u16 len = 0;

  err_t_wrap (kvb_build_common (&keys, &types, &len, ub, ub->dest, e), e);

  ASSERT (keys);
  ASSERT (types);
  ASSERT (len > 1);

  dest->keys = keys;
  dest->types = types;
  dest->len = len;
  return SUCCESS;
}

/* --------------------------------------------------------------------------
 * Unit tests for kvt_builder (struct + union builder)
 * Uses the project test helpers:
 *    – test_assert_int_equal(actual, expected)
 *    – test_fail_if(expr)
 *    – test_fail_if_null(expr)
 * -------------------------------------------------------------------------- */

TEST (kvt_builder)
{
  error err = error_create (NULL);
  u8 _alloc[2048];
  u8 _dest[2048];

  /* provide two fixed‑size allocators for nodes + strings */
  lalloc alloc = lalloc_create_from (_alloc);
  lalloc dest = lalloc_create_from (_dest);

  /* 0. freshly‑created builder must be clean */
  kvt_builder kb = kvb_create (&alloc, &dest);
  test_assert_int_equal (kb.klen, 0);
  test_assert_int_equal (kb.tlen, 0);
  test_fail_if (kb.head != NULL);

  // 1. accept first key "id"
  string key_id = unsafe_cstrfrom ("id");
  test_assert_int_equal (kvb_accept_key (&kb, key_id, &err), SUCCESS);

  // 2. duplicate key "id" must fail
  test_assert_int_equal (kvb_accept_key (&kb, key_id, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 3. accept a type for that key (u32)
  type t_u32 = (type){ .type = T_PRIM, .p = U32 };
  test_assert_int_equal (kvb_accept_type (&kb, t_u32, &err), SUCCESS);

  // 4. accept second key/value pair ("name", string)
  string key_name = unsafe_cstrfrom ("name");
  test_assert_int_equal (kvb_accept_key (&kb, key_name, &err), SUCCESS);
  type t_str = (type){ .type = T_PRIM, .p = I32 };
  test_assert_int_equal (kvb_accept_type (&kb, t_str, &err), SUCCESS);

  // 6. mismatched key/type counts → build must fail
  string key_extra = unsafe_cstrfrom ("extra");
  test_assert_int_equal (kvb_accept_key (&kb, key_extra, &err), SUCCESS); // klen=3, tlen=2
  struct_t st_fail = { 0 };
  test_assert_int_equal (kvb_struct_t_build (&st_fail, &kb, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 7. add matching type so counts align
  type t_f32 = (type){ .type = T_PRIM, .p = F32 };
  test_assert_int_equal (kvb_accept_type (&kb, t_f32, &err), SUCCESS);

  // 8. successful struct build
  struct_t st = { 0 };
  test_assert_int_equal (kvb_struct_t_build (&st, &kb, &err), SUCCESS);
  test_assert_int_equal (st.len, 3);
  test_fail_if_null (st.keys);
  test_fail_if_null (st.types);

  // 9. ensure key order preserved (id, name, extra)
  test_assert_int_equal (string_equal (st.keys[0], key_id), true);
  test_assert_int_equal (string_equal (st.keys[1], key_name), true);
  test_assert_int_equal (string_equal (st.keys[2], key_extra), true);

  // 10. ensure type mapping correct
  test_assert_int_equal (st.types[0].p, t_u32.p);
  test_assert_int_equal (st.types[1].p, t_str.p);
  test_assert_int_equal (st.types[2].p, t_f32.p);

  // 11. union build with same builder should also succeed
  union_t un = { 0 };
  test_assert_int_equal (kvb_union_t_build (&un, &kb, &err), SUCCESS);
  test_assert_int_equal (un.len, st.len);
}
