#include "compiler/ast/type/builders/enum.h"

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h"  // TEST
#include "core/errors/error.h" // err_t

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

  node->key = key;
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

TEST (enum_builder)
{
  error err = error_create (NULL);
  u8 _alloc[2048];
  u8 _dest[2048];

  /* provide two simple heap allocators for builder + strings */
  lalloc alloc = lalloc_create_from (_alloc);
  lalloc dest = lalloc_create_from (_dest);

  /* 0. freshly‑created builder must be clean */
  enum_builder eb = enb_create (&alloc, &dest);
  test_fail_if (eb.head != NULL);

  // 1. rejecting empty key
  test_assert_int_equal (enb_accept_key (&eb, (string){ 0 }, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 2. accept first key "A"
  string A = unsafe_cstrfrom ("A");
  test_assert_int_equal (enb_accept_key (&eb, A, &err), SUCCESS);

  // 3. duplicate key "A" must fail
  test_assert_int_equal (enb_accept_key (&eb, A, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;

  // 4. accept a second key "B"
  string B = unsafe_cstrfrom ("B");
  test_assert_int_equal (enb_accept_key (&eb, B, &err), SUCCESS);

  // 5. build now that we have two keys
  enum_t en = { 0 };
  test_assert_int_equal (enb_build (&en, &eb, &err), SUCCESS);
  test_assert_int_equal (en.len, 2);
  test_fail_if_null (en.keys);
  test_assert_int_equal (string_equal (en.keys[0], A) || string_equal (en.keys[1], A), true);
  test_assert_int_equal (string_equal (en.keys[0], B) || string_equal (en.keys[1], B), true);

  lalloc_reset (&alloc);
  lalloc_reset (&dest);

  // 6. build with empty builder must fail
  enum_builder empty = enb_create (&alloc, &dest);
  enum_t en2 = { 0 };
  test_assert_int_equal (enb_build (&en2, &empty, &err), ERR_INVALID_ARGUMENT);
  err.cause_code = SUCCESS;
}
