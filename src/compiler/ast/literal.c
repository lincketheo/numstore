#include "compiler/ast/literal.h"

#include "core/dev/assert.h"
#include "core/dev/testing.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/logging.h"
#include "core/intf/math.h"
#include "core/utils/numbers.h"
#include "core/utils/strings.h"

static const char *ARRAY_BUILDER_TAG = "Array Value Builder";
static const char *OBJECT_BUILDER_TAG = "Object Value Builder";

///////////////////////////
/// Object and array literal types

i32
object_t_snprintf (char *str, u32 size, const object *st)
{
  (void)str;
  (void)size;
  (void)st;
  return -1;
}

bool
object_equal (const object *left, const object *right)
{
  if (left->len != right->len)
    {
      return false;
    }

  for (u32 i = 0; i < left->len; ++i)
    {
      if (!string_equal (left->keys[i], right->keys[i]))
        {
          return false;
        }
      if (!literal_equal (&left->literals[i], &right->literals[i]))
        {
          return false;
        }
    }

  return true;
}

err_t
object_plus (object *dest, const object *right, lalloc *alloc, error *e)
{
  // Check for duplicate keys
  const string *duplicate = strings_are_disjoint (dest->keys, dest->len, right->keys, right->len);
  if (duplicate != NULL)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Cannot merge two objects with duplicate keys: %.*s",
          duplicate->len, duplicate->data);
    }

  u32 len = dest->len + right->len;

  string *keys = lmalloc (alloc, len, sizeof *keys);
  literal *literals = lmalloc (alloc, len, sizeof *literals);
  if (literals == NULL || keys == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate dest object");
      return e->cause_code;
    }

  // Copy over literals
  i_memcpy (literals, dest->literals, dest->len * sizeof *dest->literals);
  i_memcpy (literals + dest->len, right->literals, right->len * sizeof *right->literals);

  // Copy over keys
  i_memcpy (keys, dest->keys, dest->len * sizeof *dest->keys);
  i_memcpy (keys + dest->len, right->keys, right->len * sizeof *right->keys);

  dest->len = len;

  return SUCCESS;
}

bool
array_equal (const array *left, const array *right)
{
  if (left->len != right->len)
    {
      return false;
    }

  for (u32 i = 0; i < left->len; ++i)
    {
      if (!literal_equal (&left->literals[i], &right->literals[i]))
        {
          return false;
        }
    }

  return true;
}

err_t
array_plus (array *dest, const array *right, lalloc *alloc, error *e)
{
  u32 len = dest->len + right->len;
  literal *literals = lmalloc (alloc, len, sizeof *literals);
  if (literals == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate literals");
      return e->cause_code;
    }

  i_memcpy (literals, dest->literals, dest->len * sizeof *dest->literals);
  i_memcpy (literals + dest->len, right->literals, right->len * sizeof *right->literals);
  dest->len = len;

  return SUCCESS;
}

///////////////////////////
/// Literal

const char *
literal_t_tostr (literal_t t)
{
  switch (t)
    {
      // Composite
    case LT_OBJECT:
      return "LT_OBJECT";
    case LT_ARRAY:
      return "LT_ARRAY";

      // Simple
    case LT_STRING:
      return "LT_STRING";
    case LT_INTEGER:
      return "LT_INTEGER";
    case LT_DECIMAL:
      return "LT_DECIMAL";
    case LT_COMPLEX:
      return "LT_COMPLEX";
    case LT_BOOL:
      return "LT_BOOL";
    default:
      UNREACHABLE ();
    }
}

bool
literal_equal (const literal *left, const literal *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
      // Composite
    case LT_OBJECT:
      {
        return object_equal (&left->obj, &right->obj);
      }
    case LT_ARRAY:
      {
        return array_equal (&left->arr, &right->arr);
      }

      // Simple
    case LT_STRING:
      {
        return string_equal (left->str, right->str);
      }
    case LT_INTEGER:
      {
        return left->integer == right->integer;
      }
    case LT_DECIMAL:
      {
        return left->decimal == right->decimal;
      }
    case LT_COMPLEX:
      {
        return left->cplx == right->cplx;
      }
    case LT_BOOL:
      {
        return left->bl == right->bl;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

///////////////////////////
/// Object / Array builders

DEFINE_DBG_ASSERT_I (object_builder, object_builder, a)
{
  ASSERT (a);
}

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
          OBJECT_BUILDER_TAG, key.len, key.data);
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
              "%s: Failed to allocate key-literal node", OBJECT_BUILDER_TAG);
        }
      llnode_init (&node->link);
      node->v = (literal){ 0 };

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
objb_accept_literal (object_builder *ub, literal t, error *e)
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
              "%s: Failed to allocate key-literal node", OBJECT_BUILDER_TAG);
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
    literal **out_types,
    u16 *out_len,
    object_builder *ub,
    lalloc *onto,
    error *e)
{
  if (ub->klen == 0)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Expecting at least one key", OBJECT_BUILDER_TAG);
    }
  if (ub->klen != ub->tlen)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s: Must have same number of keys and literals", OBJECT_BUILDER_TAG);
    }

  *out_keys = lmalloc (onto, ub->klen, sizeof **out_keys);
  *out_types = lmalloc (onto, ub->tlen, sizeof **out_types);
  if (!*out_keys || !*out_types)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate output arrays", OBJECT_BUILDER_TAG);
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
  literal *literals = NULL;
  u16 len = 0;

  err_t_wrap (objb_build_common (&keys, &literals, &len, ub, ub->dest, e), e);

  ASSERT (keys);
  ASSERT (literals);

  dest->keys = keys;
  dest->literals = literals;
  dest->len = len;
  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (array_builder, array_builder, a)
{
  ASSERT (a);
}

array_builder
arb_create (lalloc *work, lalloc *dest)
{
  array_builder builder = {
    .head = NULL,
    .work = work,
    .dest = dest,
  };

  array_builder_assert (&builder);

  return builder;
}

err_t
arb_accept_literal (array_builder *o, literal v, error *e)
{
  array_builder_assert (o);

  u16 idx = (u16)list_length (o->head);
  llnode *slot = llnode_get_n (o->head, idx);
  array_llnode *node;

  if (slot)
    {
      node = container_of (slot, array_llnode, link);
    }
  else
    {
      node = lmalloc (o->work, 1, sizeof *node);
      if (!node)
        {
          return error_causef (e, ERR_NOMEM, "%s: allocation failed", ARRAY_BUILDER_TAG);
        }
      llnode_init (&node->link);
      if (!o->head)
        {
          o->head = &node->link;
        }
      else
        {
          list_append (&o->head, &node->link);
        }
    }

  node->v = v;
  return SUCCESS;
}

err_t
arb_build (array *dest, array_builder *b, error *e)
{
  array_builder_assert (b);
  ASSERT (dest);

  u16 length = (u16)list_length (b->head);

  literal *literals = lmalloc (b->dest, length, sizeof *literals);
  if (!literals)
    {
      return error_causef (e, ERR_NOMEM, "%s: allocation failed", ARRAY_BUILDER_TAG);
    }

  u16 i = 0;
  for (llnode *it = b->head; it; it = it->next)
    {
      array_llnode *dn = container_of (it, array_llnode, link);
      literals[i++] = dn->v;
    }

  dest->len = length;
  dest->literals = literals;

  return SUCCESS;
}

///////////////////////////
/// Expression reductions

/**
 * Enforces type coersion for number / complex for
 * the common pattern:
 *
 * dest (bool) = dest OP right
 *
 * Assuming left or right is complex, leaves
 * them unchanged (e.g. no absolute literal)
 *
 * where OP is a boolean operator
 */
#define BOOL_NUMBER_TYPE_COERCE(dest, OP, right)                      \
  do                                                                  \
    {                                                                 \
      /* int + int */                                                 \
      if (dest->type == LT_INTEGER && right->type == LT_INTEGER)      \
        {                                                             \
          dest->bl = dest->integer OP right->integer;                 \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* dec + dec */                                                 \
      else if (dest->type == LT_DECIMAL && right->type == LT_DECIMAL) \
        {                                                             \
          dest->bl = dest->decimal OP right->decimal;                 \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* cplx + dec */                                                \
      else if (dest->type == LT_COMPLEX && right->type == LT_DECIMAL) \
        {                                                             \
          dest->bl = dest->cplx OP (cf128) right->decimal;            \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* cplx + cplx */                                               \
      else if (dest->type == LT_COMPLEX && right->type == LT_COMPLEX) \
        {                                                             \
          dest->bl = dest->cplx OP right->cplx;                       \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* dec + cplx */                                                \
      else if (dest->type == LT_DECIMAL && right->type == LT_COMPLEX) \
        {                                                             \
          dest->bl = (cf128)dest->decimal OP right->cplx;             \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* int + dec */                                                 \
      else if (dest->type == LT_INTEGER && right->type == LT_DECIMAL) \
        {                                                             \
          dest->bl = (f64)dest->integer OP right->decimal;            \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* dec + int */                                                 \
      else if (dest->type == LT_DECIMAL && right->type == LT_INTEGER) \
        {                                                             \
          dest->bl = dest->decimal OP (f64) right->integer;           \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* int + cplx */                                                \
      else if (dest->type == LT_INTEGER && right->type == LT_COMPLEX) \
        {                                                             \
          dest->bl = (cf128)dest->integer OP right->cplx;             \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
                                                                      \
      /* cplx + int */                                                \
      else if (dest->type == LT_COMPLEX && right->type == LT_INTEGER) \
        {                                                             \
          dest->bl = dest->cplx OP (cf128) right->integer;            \
          dest->type = LT_BOOL;                                       \
          return SUCCESS;                                             \
        }                                                             \
    }                                                                 \
  while (0)

#define BOOL_NUMBER_TYPE_COERCE_CABS(dest, OP, right)                                   \
  do                                                                                    \
    {                                                                                   \
      /* int OP int -> bool */                                                          \
      if ((dest)->type == LT_INTEGER && (right)->type == LT_INTEGER)                    \
        {                                                                               \
          (dest)->bl = (dest)->integer OP (right)->integer;                             \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
                                                                                        \
      /* dec OP dec -> bool */                                                          \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_DECIMAL)               \
        {                                                                               \
          (dest)->bl = (dest)->decimal OP (right)->decimal;                             \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
                                                                                        \
      /* |cmplx| OP |cmplx| -> bool */                                                  \
      else if ((dest)->type == LT_COMPLEX && (right)->type == LT_COMPLEX)               \
        {                                                                               \
          (dest)->bl = i_cabs_sqrd_64 ((dest)->cplx) OP i_cabs_sqrd_64 ((right)->cplx); \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
      /* dec OP int -> bool */                                                          \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_INTEGER)               \
        {                                                                               \
          (dest)->bl = (dest)->decimal OP (right)->integer;                             \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
                                                                                        \
      /* int OP dec -> bool */                                                          \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_DECIMAL)               \
        {                                                                               \
          (dest)->bl = (dest)->integer OP (right)->decimal;                             \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
                                                                                        \
      /* int OP |cmplx| -> bool */                                                      \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_COMPLEX)               \
        {                                                                               \
          (dest)->bl = (dest)->integer OP i_cabs_sqrd_64 ((right)->cplx);               \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
                                                                                        \
      /* dec OP |cmplx| -> bool */                                                      \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_COMPLEX)               \
        {                                                                               \
          (dest)->bl = (dest)->decimal OP i_cabs_sqrd_64 ((right)->cplx);               \
          (dest)->type = LT_BOOL;                                                       \
          return SUCCESS;                                                               \
        }                                                                               \
    }                                                                                   \
  while (0)

/**
 * Enforces type coersion for number / complex for
 * the common pattern:
 *
 * dest = dest OP right
 *
 * where OP is a scalar operator
 */
#define SCALAR_NUMBER_TYPE_COERCE(dest, OP, right)                        \
  do                                                                      \
    {                                                                     \
      /* int + int */                                                     \
      if ((dest)->type == LT_INTEGER && (right)->type == LT_INTEGER)      \
        {                                                                 \
          (dest)->integer = (dest)->integer OP (right)->integer;          \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* dec + dec */                                                     \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_DECIMAL) \
        {                                                                 \
          (dest)->decimal = (dest)->decimal OP (right)->decimal;          \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* cmplx + cmplx */                                                 \
      else if ((dest)->type == LT_COMPLEX && (right)->type == LT_COMPLEX) \
        {                                                                 \
          (dest)->cplx = (dest)->cplx OP (right)->cplx;                   \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* cmplx + dec */                                                   \
      else if ((dest)->type == LT_COMPLEX && (right)->type == LT_DECIMAL) \
        {                                                                 \
          (dest)->cplx = (dest)->cplx OP (right)->decimal;                \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* cmplx + int */                                                   \
      else if ((dest)->type == LT_COMPLEX && (right)->type == LT_INTEGER) \
        {                                                                 \
          (dest)->cplx = (dest)->cplx OP (right)->integer;                \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* dec + int */                                                     \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_INTEGER) \
        {                                                                 \
          (dest)->decimal = (dest)->decimal OP (right)->integer;          \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* dec + cmplx -> promote to complex */                             \
      else if ((dest)->type == LT_DECIMAL && (right)->type == LT_COMPLEX) \
        {                                                                 \
          cf128 tmp = (dest)->decimal;                                    \
          tmp = tmp OP (right)->cplx;                                     \
          (dest)->cplx = tmp;                                             \
          (dest)->type = LT_COMPLEX;                                      \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* int + dec -> promote to decimal */                               \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_DECIMAL) \
        {                                                                 \
          (dest)->decimal = (dest)->integer OP (right)->decimal;          \
          (dest)->type = LT_DECIMAL;                                      \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* int + cmplx -> promote to complex */                             \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_COMPLEX) \
        {                                                                 \
          cf128 tmp = (dest)->integer;                                    \
          tmp = tmp OP (right)->cplx;                                     \
          (dest)->cplx = tmp;                                             \
          (dest)->type = LT_COMPLEX;                                      \
          return SUCCESS;                                                 \
        }                                                                 \
    }                                                                     \
  while (0)

#define BITMANIP_TYPE_COERCE(dest, OP, right)                             \
  do                                                                      \
    {                                                                     \
      /* bool OP bool => bool */                                          \
      if ((dest)->type == LT_BOOL && (right)->type == LT_BOOL)            \
        {                                                                 \
          (dest)->bl = (dest)->bl OP (right)->bl;                         \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* int OP bool => int */                                            \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_BOOL)    \
        {                                                                 \
          (dest)->integer = (dest)->integer OP (right)->bl;               \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* bool OP int => int */                                            \
      else if ((dest)->type == LT_BOOL && (right)->type == LT_INTEGER)    \
        {                                                                 \
          (dest)->integer = (dest)->bl OP (right)->integer;               \
          (dest)->type = LT_INTEGER;                                      \
          return SUCCESS;                                                 \
        }                                                                 \
                                                                          \
      /* int OP int => int */                                             \
      else if ((dest)->type == LT_INTEGER && (right)->type == LT_INTEGER) \
        {                                                                 \
          (dest)->integer = (dest)->integer OP (right)->integer;          \
          return SUCCESS;                                                 \
        }                                                                 \
    }                                                                     \
  while (0)

#define ERR_UNSUPPORTED_BIN_OP(left, right, op)   \
  error_causef (                                  \
      e, ERR_SYNTAX,                              \
      "Unsupported operation type: %s for %s %s", \
      op, literal_t_tostr (left->type),           \
      literal_t_tostr (right->type))

#define ERR_UNSUPPORTED_UN_OP(dest, op)        \
  error_causef (                               \
      e, ERR_SYNTAX,                           \
      "Unsupported operation type: %s for %s", \
      op, literal_t_tostr (dest->type))

static inline bool
literal_truthy (const literal *v)
{
  switch (v->type)
    {
    case LT_BOOL:
      {
        return v->bl;
      }
    case LT_INTEGER:
      {
        return v->integer != 0.0;
      }
    case LT_DECIMAL:
      {
        return v->decimal != 0.0;
      }
    case LT_COMPLEX:
      {
        return i_cabs_sqrd_64 (v->cplx) != 0.0;
      }
    case LT_STRING:
      {
        return v->str.len != 0;
      }
    case LT_OBJECT:
      {
        return v->obj.len != 0;
      }
    case LT_ARRAY:
      {
        return v->arr.len != 0;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

TEST (literal_truthy)
{
  literal v;

  v = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_INTEGER, .integer = 0 };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_INTEGER, .integer = 3 };
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_DECIMAL, .decimal = 0.0 };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_DECIMAL, .decimal = 3.14 };
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_COMPLEX, .cplx = 0 + 0 * I };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_COMPLEX, .cplx = 3 + 4 * I }; // magnitude = 5
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_STRING, .str = { .data = "", .len = 0 } };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_STRING, .str = { .data = "hi", .len = 2 } };
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_OBJECT, .obj = { .len = 0 } };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_OBJECT, .obj = { .len = 2 } };
  test_assert (literal_truthy (&v));

  v = (literal){ .type = LT_ARRAY, .arr = { .len = 0 } };
  test_assert (!literal_truthy (&v));

  v = (literal){ .type = LT_ARRAY, .arr = { .len = 5 } };
  test_assert (literal_truthy (&v));
}

err_t
literal_plus_literal (literal *dest, const literal *right, lalloc *alc, error *e)
{
  // First check for number / complex
  SCALAR_NUMBER_TYPE_COERCE (dest, +, right);

  // ARRAY + ARRAY
  if (dest->type == LT_ARRAY && right->type == LT_ARRAY)
    {
      return array_plus (&dest->arr, &right->arr, alc, e);
    }

  // OBJECT + OBJECT
  else if (dest->type == LT_OBJECT && right->type == LT_OBJECT)
    {
      return object_plus (&dest->obj, &right->obj, alc, e);
    }

  // STRING + STRING
  else if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      string next = string_plus (dest->str, right->str, alc, e);
      if (e->cause_code)
        {
          return e->cause_code;
        }
      dest->str = next;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "+");
}

#ifndef NTEST
typedef struct
{
  literal_t lhs, rhs;
} literal_pair;
#endif

TEST (literal_plus_literal)
{
  u8 data[2048];
  lalloc alc = lalloc_create_from (data);
  error err = error_create (NULL);

  // String + String
  literal a = { .type = LT_STRING, .str = unsafe_cstrfrom ("foo") };
  literal b = { .type = LT_STRING, .str = unsafe_cstrfrom ("bar") };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.str.len == 6);
  test_assert (memcmp (a.str.data, "foobar", 6) == 0);

  // Integer + Integer
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 7);

  // Decimal + Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.25 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.75 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 4.0f) < 1e-12);

  // Integer + Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 5.5 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 7.5f) < 1e-12);

  // Decimal + Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.5 };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 5.5f) < 1e-12);

  // Complex + Complex
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 4.0);
  test_assert (i_cimag_64 (a.cplx) == 6.0);

  // Complex + Integer
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 6.0);
  test_assert (i_cimag_64 (a.cplx) == 2.0);

  // Complex + Decimal
  a = (literal){ .type = LT_COMPLEX, .cplx = -1.0 + 0.5 * I };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 1.5);
  test_assert (i_cimag_64 (a.cplx) == 0.5);

  // Integer + Complex
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 0.0 + 3.0 * I };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 2.0);
  test_assert (i_cimag_64 (a.cplx) == 3.0);

  // Decimal + Complex
  a = (literal){ .type = LT_DECIMAL, .decimal = 4.75 };
  b = (literal){ .type = LT_COMPLEX, .cplx = -1.0 + 0.25 * I };
  test_assert (literal_plus_literal (&a, &b, &alc, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 3.75);
  test_assert (i_cimag_64 (a.cplx) == 0.25);

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_BOOL },
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_plus_literal (&a, &b, &alc, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected plus to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_minus_literal (literal *dest, const literal *right, error *e)
{
  SCALAR_NUMBER_TYPE_COERCE (dest, -, right);
  return ERR_UNSUPPORTED_BIN_OP (dest, right, "-");
}

TEST (literal_minus_literal)
{
  error err = error_create (NULL);

  // Integer - Integer
  literal a = { .type = LT_INTEGER, .integer = 5 };
  literal b = { .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 3);

  // Decimal - Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 5.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.25 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 4.25f) < 1e-12);

  // Integer - Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 0.5 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 2.5f) < 1e-12);

  // Decimal - Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 4.0 };
  b = (literal){ .type = LT_INTEGER, .integer = 1 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 3.0f) < 1e-12);

  // Complex - Complex
  a = (literal){ .type = LT_COMPLEX, .cplx = 5.0 + 6.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 4.0);
  test_assert (i_cimag_64 (a.cplx) == 4.0);

  // Complex - Integer
  a = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  b = (literal){ .type = LT_INTEGER, .integer = 1 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 1.0);
  test_assert (i_cimag_64 (a.cplx) == 3.0);

  // Complex - Decimal
  a = (literal){ .type = LT_COMPLEX, .cplx = 4.0 + 2.0 * I };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 2.5);
  test_assert (i_cimag_64 (a.cplx) == 2.0);

  // Integer - Complex
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 2.0);
  test_assert (i_cimag_64 (a.cplx) == -1.0);

  // Decimal - Complex
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 0.5 * I };
  test_assert (literal_minus_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 2.0);
  test_assert (i_cimag_64 (a.cplx) == -0.5);

  // Object - Object
  // Array - Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_BOOL },
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_minus_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected minus to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_star_literal (literal *dest, const literal *right, error *e)
{
  SCALAR_NUMBER_TYPE_COERCE (dest, *, right);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "*");
}

TEST (literal_star_literal)
{
  error err = error_create (NULL);

  // Integer * Integer
  literal a = { .type = LT_INTEGER, .integer = 3 };
  literal b = { .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 12);

  // Decimal * Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 3.0f) < 1e-12);

  // Integer * Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 9.0f) < 1e-12);

  // Decimal * Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 5.0f) < 1e-12);

  // Complex * Complex
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == -4.0);
  test_assert (i_cimag_64 (a.cplx) == 7.0);

  // Complex * Integer
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.5 + 0.5 * I };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 3.0);
  test_assert (i_cimag_64 (a.cplx) == 1.0);

  // Complex * Decimal
  a = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 4.0);
  test_assert (i_cimag_64 (a.cplx) == 6.0);

  // Integer * Complex
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 6.0);
  test_assert (i_cimag_64 (a.cplx) == 8.0);

  // Decimal * Complex
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 4.0 + 2.0 * I };
  test_assert (literal_star_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 6.0);
  test_assert (i_cimag_64 (a.cplx) == 3.0);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_BOOL },
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_star_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected star to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_slash_literal (literal *dest, const literal *right, error *e)
{
  SCALAR_NUMBER_TYPE_COERCE (dest, /, right);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "/");
}

TEST (literal_slash_literal)
{
  error err = error_create (NULL);

  // Integer / Integer
  literal a = { .type = LT_INTEGER, .integer = 10 };
  literal b = { .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 5);

  // Decimal / Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 9.0 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 3.0f) < 1e-12);

  // Integer / Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 6 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 3.0f) < 1e-12);

  // Decimal / Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  b = (literal){ .type = LT_INTEGER, .integer = 3 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (i_fabs_32 (a.decimal - 1.5f) < 1e-12);

  // Complex / Complex
  a = (literal){ .type = LT_COMPLEX, .cplx = 4.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) != 0.0 || i_cimag_64 (a.cplx) != 0.0); // sanity check

  // Complex / Integer
  a = (literal){ .type = LT_COMPLEX, .cplx = 6.0 + 2.0 * I };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 3.0);
  test_assert (i_cimag_64 (a.cplx) == 1.0);

  // Complex / Decimal
  a = (literal){ .type = LT_COMPLEX, .cplx = 8.0 + 4.0 * I };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == 4.0);
  test_assert (i_cimag_64 (a.cplx) == 2.0);

  // Integer / Complex
  a = (literal){ .type = LT_INTEGER, .integer = 4 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);

  // Decimal / Complex
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 - 1.0 * I };
  test_assert (literal_slash_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_BOOL },
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_slash_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected slash to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_equal_equal_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE (dest, ==, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = string_equal (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_OBJECT && right->type == LT_OBJECT)
    {
      dest->bl = object_equal (&dest->obj, &right->obj);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_ARRAY && right->type == LT_ARRAY)
    {
      dest->bl = array_equal (&dest->arr, &right->arr);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_BOOL && right->type == LT_BOOL)
    {
      dest->bl = dest->bl == right->bl;
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "==");
}

TEST (literal_equal_equal_literal)
{
  error err = error_create (NULL);

  // TRUE == TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // TRUE == FALSE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // FALSE == FALSE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer == Integer (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_INTEGER, .integer = 3 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer == Integer (not equal)
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal == Decimal (equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal == Decimal (not equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.0 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer == Decimal (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal == Integer (not equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex == Complex (equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex == Complex (not equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 1.0 * I };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String == String (equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String == String (not equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("world") };
  test_assert (literal_equal_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_equal_equal_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected equal equal to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_bang_equal_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE (dest, !=, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = !string_equal (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_OBJECT && right->type == LT_OBJECT)
    {
      dest->bl = !object_equal (&dest->obj, &right->obj);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_ARRAY && right->type == LT_ARRAY)
    {
      dest->bl = !array_equal (&dest->arr, &right->arr);
      dest->type = LT_BOOL;
      return SUCCESS;
    }
  else if (dest->type == LT_BOOL && right->type == LT_BOOL)
    {
      dest->bl = dest->bl != right->bl;
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "!=");
}

TEST (literal_bang_equal_literal)
{
  error err = error_create (NULL);

  // TRUE != TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // TRUE != FALSE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // FALSE != FALSE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer != Integer (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer != Integer (not equal)
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_INTEGER, .integer = 7 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal != Decimal (equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal != Decimal (not equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.0 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer != Decimal (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal != Integer (not equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex != Complex (equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex != Complex (not equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 2.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 2.0 * I };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String != String (equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("same") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("same") };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String != String (not equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("b") };
  test_assert (literal_bang_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_bang_equal_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected bang equal to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_greater_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE_CABS (dest, >, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = string_greater_string (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, ">");
}

TEST (literal_greater_literal)
{
  error err = error_create (NULL);

  // TRUE > FALSE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = false };
  test_assert (literal_greater_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE > TRUE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_greater_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Integer > Integer (true)
  a = (literal){ .type = LT_INTEGER, .integer = 10 };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer > Integer (false)
  a = (literal){ .type = LT_INTEGER, .integer = 1 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal > Decimal (true)
  a = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal > Decimal (false)
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.25 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer > Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal > Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex > Complex (by magnitude: true)
  a = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I }; // mag = 5
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I }; // mag ≈ 1.41
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex > Complex (false)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String > String (true)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("zebra") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("apple") };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String > String (false)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("z") };
  test_assert (literal_greater_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_greater_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected greater to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_greater_equal_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE_CABS (dest, >=, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = string_greater_equal_string (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, ">=");
}

TEST (literal_greater_equal_literal)
{
  error err = error_create (NULL);

  // TRUE >= TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE >= FALSE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE >= TRUE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Integer >= Integer (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer >= Integer (true)
  a = (literal){ .type = LT_INTEGER, .integer = 10 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer >= Integer (false)
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_INTEGER, .integer = 7 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal >= Decimal (equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 5.0 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 5.0 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal >= Decimal (true)
  a = (literal){ .type = LT_DECIMAL, .decimal = 8.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 4.1 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal >= Decimal (false)
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.14 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 9.8 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex >= Complex (equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex >= Complex (true)
  a = (literal){ .type = LT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 0.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex >= Complex (false)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 4.0 + 3.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String >= String (equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("test") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("test") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String >= String (true)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("z") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String >= String (false)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("b") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);
}

err_t
literal_less_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE_CABS (dest, <, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = string_less_string (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "<");
}

TEST (literal_less_literal)
{
  error err = error_create (NULL);

  // TRUE >= TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE >= FALSE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE >= TRUE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Integer >= Integer (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer >= Integer (true)
  a = (literal){ .type = LT_INTEGER, .integer = 10 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer >= Integer (false)
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_INTEGER, .integer = 7 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal >= Decimal (equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal >= Decimal (true)
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal >= Decimal (false)
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.0 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer >= Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal >= Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.0 };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex >= Complex (equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex >= Complex (true)
  a = (literal){ .type = LT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 0.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex >= Complex (false)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 4.0 + 3.0 * I };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String >= String (equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("test") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("test") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String >= String (true)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("z") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String >= String (false)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("a") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("b") };
  test_assert (literal_greater_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_greater_equal_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected greater equal to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_less_equal_literal (literal *dest, const literal *right, error *e)
{
  BOOL_NUMBER_TYPE_COERCE_CABS (dest, <=, right);

  if (dest->type == LT_STRING && right->type == LT_STRING)
    {
      dest->bl = string_less_equal_string (dest->str, right->str);
      dest->type = LT_BOOL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "<=");
}

TEST (literal_less_equal_literal)
{
  error err = error_create (NULL);

  // TRUE <= TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_less_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // TRUE <= FALSE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_less_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // FALSE <= TRUE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_less_equal_literal (&a, &b, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // Integer <= Integer (equal)
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer <= Integer (true)
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_INTEGER, .integer = 7 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Integer <= Integer (false)
  a = (literal){ .type = LT_INTEGER, .integer = 9 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal <= Decimal (equal)
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal <= Decimal (true)
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.25 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal <= Decimal (false)
  a = (literal){ .type = LT_DECIMAL, .decimal = 3.0 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 1.0 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer <= Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 2 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal <= Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 4.5 };
  b = (literal){ .type = LT_INTEGER, .integer = 4 };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex <= Complex (equal)
  a = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex <= Complex (true)
  a = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex <= Complex (false)
  a = (literal){ .type = LT_COMPLEX, .cplx = 5.0 + 0.0 * I };
  b = (literal){ .type = LT_COMPLEX, .cplx = 1.0 + 1.0 * I };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String <= String (equal)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("abc") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("abc") };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String <= String (true)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("abc") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("bcd") };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String <= String (false)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("xyz") };
  b = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("abc") };
  test_assert (literal_less_equal_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Object + Object
  // Array + Array

  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_less_equal_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected less equal to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

// dest = dest ^ right
err_t
literal_caret_literal (literal *dest, const literal *right, error *e)
{
  BITMANIP_TYPE_COERCE (dest, ^, right);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "^");
}

TEST (literal_caret_literal)
{
  error err = error_create (NULL);

  // true ^ true -> false
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // true ^ false -> true
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // false ^ true -> true
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // int ^ int
  a = (literal){ .type = LT_INTEGER, .integer = 6 }; // 110
  b = (literal){ .type = LT_INTEGER, .integer = 3 }; // 011
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 5); // 101

  // int ^ bool
  a = (literal){ .type = LT_INTEGER, .integer = 6 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 7);

  // bool ^ int
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_caret_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 3);

  // invalid combos
  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_DECIMAL },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_STRING },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },

    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_DECIMAL },
    { LT_INTEGER, LT_COMPLEX },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_INTEGER },
    { LT_DECIMAL, LT_DECIMAL },
    { LT_DECIMAL, LT_COMPLEX },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_INTEGER },
    { LT_COMPLEX, LT_DECIMAL },
    { LT_COMPLEX, LT_COMPLEX },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_OBJECT },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_OBJECT },
    { LT_ARRAY, LT_ARRAY },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_caret_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected caret to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

// dest = dest % right
err_t
literal_mod_literal (literal *dest, const literal *right, error *e)
{
  // int % int
  if (dest->type == LT_INTEGER && right->type == LT_INTEGER)
    {
      dest->integer = py_mod_i32 (dest->integer, right->integer);
      return SUCCESS;
    }

  // dec % dec
  else if (dest->type == LT_DECIMAL && right->type == LT_DECIMAL)
    {
      dest->decimal = py_mod_f32 (dest->decimal, right->decimal);
      return SUCCESS;
    }

  // dec % int
  else if (dest->type == LT_DECIMAL && right->type == LT_INTEGER)
    {
      dest->decimal = py_mod_f32 (dest->decimal, (float)right->integer);
      return SUCCESS;
    }

  // int % dec
  else if (dest->type == LT_INTEGER && right->type == LT_DECIMAL)
    {
      dest->decimal = py_mod_f32 ((f32)dest->integer, right->decimal);
      dest->type = LT_DECIMAL;
      return SUCCESS;
    }

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "%");
}

TEST (literal_mod_literal)
{
  error err = error_create (NULL);

  // Integer % Integer
  literal a = { .type = LT_INTEGER, .integer = 10 };
  literal b = { .type = LT_INTEGER, .integer = 3 };
  test_assert (literal_mod_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 1);

  // Decimal % Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 5.5f };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0f };
  test_assert (literal_mod_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (a.decimal == 1.5f);

  // Decimal % Integer
  a = (literal){ .type = LT_DECIMAL, .decimal = 5.5f };
  b = (literal){ .type = LT_INTEGER, .integer = 2 };
  test_assert (literal_mod_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (a.decimal == 1.5f);

  // Integer % Decimal
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_DECIMAL, .decimal = 2.0f };
  test_assert (literal_mod_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (a.decimal == 1.0f);

  // Invalid combos
  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_INTEGER },
    { LT_BOOL, LT_DECIMAL },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_STRING },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },

    { LT_INTEGER, LT_BOOL },
    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_COMPLEX },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_COMPLEX },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_INTEGER },
    { LT_COMPLEX, LT_DECIMAL },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_mod_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected mod to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

// dest = dest | right
err_t
literal_pipe_literal (literal *dest, const literal *right, error *e)
{
  BITMANIP_TYPE_COERCE (dest, |, right);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "|");
}

TEST (literal_pipe_literal)
{
  error err = error_create (NULL);

  // TRUE | TRUE -> TRUE
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // TRUE | FALSE -> TRUE
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // FALSE | FALSE -> FALSE
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // INT | INT -> INT
  a.type = LT_INTEGER;
  a.integer = 10; // 0b1010
  b.type = LT_INTEGER;
  b.integer = 12; // 0b1100
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 14); // 0b1110

  // INT | BOOL -> INT
  a.type = LT_INTEGER;
  a.integer = 8; // 0b1000
  b.type = LT_BOOL;
  b.bl = true;
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 9); // 0b1001

  // BOOL | INT -> INT
  a.type = LT_BOOL;
  a.bl = true;
  b.type = LT_INTEGER;
  b.integer = 4; // 0b0100
  test_assert (literal_pipe_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 5); // 0b0101

  // Invalid combos
  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_DECIMAL },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },
    { LT_STRING, LT_COMPLEX },
    { LT_STRING, LT_STRING },
    { LT_STRING, LT_OBJECT },
    { LT_STRING, LT_ARRAY },

    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_DECIMAL },
    { LT_INTEGER, LT_COMPLEX },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_INTEGER },
    { LT_DECIMAL, LT_DECIMAL },
    { LT_DECIMAL, LT_COMPLEX },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_OBJECT },
    { LT_DECIMAL, LT_ARRAY },

    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_INTEGER },
    { LT_COMPLEX, LT_DECIMAL },
    { LT_COMPLEX, LT_COMPLEX },
    { LT_COMPLEX, LT_STRING },
    { LT_COMPLEX, LT_OBJECT },
    { LT_COMPLEX, LT_ARRAY },

    { LT_OBJECT, LT_BOOL },
    { LT_OBJECT, LT_INTEGER },
    { LT_OBJECT, LT_DECIMAL },
    { LT_OBJECT, LT_COMPLEX },
    { LT_OBJECT, LT_STRING },
    { LT_OBJECT, LT_OBJECT },
    { LT_OBJECT, LT_ARRAY },

    { LT_ARRAY, LT_BOOL },
    { LT_ARRAY, LT_INTEGER },
    { LT_ARRAY, LT_DECIMAL },
    { LT_ARRAY, LT_COMPLEX },
    { LT_ARRAY, LT_STRING },
    { LT_ARRAY, LT_OBJECT },
    { LT_ARRAY, LT_ARRAY },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_pipe_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected pipe to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

// dest = dest || right
void
literal_pipe_pipe_literal (literal *dest, const literal *right)
{
  bool bleft = literal_truthy (dest);
  bool bright = literal_truthy (right);

  dest->bl = bleft || bright;
  dest->type = LT_BOOL;
}

TEST (literal_pipe_pipe_literal)
{
  // true || true -> true
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // true || false -> true
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // false || true -> true
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // false || false -> false
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // integer (0) || false -> false
  a = (literal){ .type = LT_INTEGER, .integer = 0 };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // integer (3) || false -> true
  a = (literal){ .type = LT_INTEGER, .integer = 3 };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // decimal (0.0) || false -> false
  a = (literal){ .type = LT_DECIMAL, .decimal = 0.0 };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // decimal (1.5) || false -> true
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // complex (0+0i) || false -> false
  a = (literal){ .type = LT_COMPLEX, .cplx = 0.0 + 0.0 * I };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // complex (2+3i) || false -> true
  a = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // string ("") || false -> false
  a = (literal){ .type = LT_STRING, .str = { .data = "", .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // string ("hello") || false -> true
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // array (empty) || false -> false
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // array (non-empty) || false -> true
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 1 } };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // object (empty) || false -> false
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // object (non-empty) || false -> true
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 1 } };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_pipe_pipe_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);
}

// dest = dest & right
err_t
literal_ampersand_literal (literal *dest, const literal *right, error *e)
{
  BITMANIP_TYPE_COERCE (dest, &, right);

  return ERR_UNSUPPORTED_BIN_OP (dest, right, "&");
}

TEST (literal_ampersand_literal)
{
  error err = error_create (NULL);

  // BOOL & BOOL
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = false };
  test_assert (literal_ampersand_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == (true & false));

  // INT & BOOL
  a = (literal){ .type = LT_INTEGER, .integer = 6 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_ampersand_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == (6 & 1));

  // BOOL & INT
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_INTEGER, .integer = 3 };
  test_assert (literal_ampersand_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == (1 & 3));

  // INT & INT
  a = (literal){ .type = LT_INTEGER, .integer = 12 };
  b = (literal){ .type = LT_INTEGER, .integer = 10 };
  test_assert (literal_ampersand_literal (&a, &b, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == (12 & 10));

  // Invalid combinations
  literal_pair invalid_combos[] = {
    { LT_BOOL, LT_STRING },
    { LT_BOOL, LT_COMPLEX },
    { LT_BOOL, LT_OBJECT },
    { LT_BOOL, LT_ARRAY },
    { LT_BOOL, LT_DECIMAL },

    { LT_STRING, LT_BOOL },
    { LT_STRING, LT_INTEGER },
    { LT_STRING, LT_DECIMAL },

    { LT_INTEGER, LT_STRING },
    { LT_INTEGER, LT_COMPLEX },
    { LT_INTEGER, LT_OBJECT },
    { LT_INTEGER, LT_ARRAY },
    { LT_INTEGER, LT_DECIMAL },

    { LT_COMPLEX, LT_INTEGER },
    { LT_COMPLEX, LT_BOOL },
    { LT_COMPLEX, LT_STRING },

    { LT_DECIMAL, LT_BOOL },
    { LT_DECIMAL, LT_STRING },
    { LT_DECIMAL, LT_INTEGER },
    { LT_DECIMAL, LT_COMPLEX },

    { LT_OBJECT, LT_OBJECT },
    { LT_ARRAY, LT_ARRAY },
  };

  for (u32 i = 0; i < sizeof (invalid_combos) / sizeof (invalid_combos[0]); i++)
    {
      a = (literal){ .type = invalid_combos[i].lhs };
      b = (literal){ .type = invalid_combos[i].rhs };
      err_t ret = literal_ampersand_literal (&a, &b, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected ampersand to return syntax error: %s %s\n", literal_t_tostr (a.type), literal_t_tostr (b.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

// dest = dest && right
void
literal_ampersand_ampersand_literal (literal *dest, const literal *right)
{
  bool bleft = literal_truthy (dest);
  bool bright = literal_truthy (right);

  dest->bl = bleft && bright;
  dest->type = LT_BOOL;
}

TEST (literal_ampersand_ampersand_literal)
{
  // true && true -> true
  literal a = { .type = LT_BOOL, .bl = true };
  literal b = { .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // true && false -> false
  a = (literal){ .type = LT_BOOL, .bl = true };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // false && true -> false
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // false && false -> false
  a = (literal){ .type = LT_BOOL, .bl = false };
  b = (literal){ .type = LT_BOOL, .bl = false };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // integer (0) && true -> false
  a = (literal){ .type = LT_INTEGER, .integer = 0 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // integer (5) && true -> true
  a = (literal){ .type = LT_INTEGER, .integer = 5 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // decimal (0.0) && true -> false
  a = (literal){ .type = LT_DECIMAL, .decimal = 0.0 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // decimal (1.5) && true -> true
  a = (literal){ .type = LT_DECIMAL, .decimal = 1.5 };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // complex (0+0i) && true -> false
  a = (literal){ .type = LT_COMPLEX, .cplx = 0.0 + 0.0 * I };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // complex (2+3i) && true -> true
  a = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // string ("") && true -> false
  a = (literal){ .type = LT_STRING, .str = { .data = "", .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // string ("hello") && true -> true
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // array (empty) && true -> false
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // array (non-empty) && true -> true
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 1 } };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // object (empty) && true -> false
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 0 } };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // object (non-empty) && true -> true
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 1 } };
  b = (literal){ .type = LT_BOOL, .bl = true };
  literal_ampersand_ampersand_literal (&a, &b);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);
}

// dest = ~dest
err_t
literal_not (literal *dest, error *e)
{
  if (dest->type == LT_INTEGER)
    {
      dest->integer = ~dest->integer;
      return SUCCESS;
    }
  else if (dest->type == LT_BOOL)
    {
      dest->integer = ~(dest->bl ? 1 : 0);
      dest->type = LT_INTEGER;
      return SUCCESS;
    }
  return ERR_UNSUPPORTED_UN_OP (dest, "~");
}

TEST (literal_not)
{
  error err = error_create (NULL);
  literal a;

  // INT ~ 0 -> -1
  a = (literal){ .type = LT_INTEGER, .integer = 0 };
  test_assert (literal_not (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == -1);

  // INT ~ -1 -> 0
  a = (literal){ .type = LT_INTEGER, .integer = -1 };
  test_assert (literal_not (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 0);

  // BOOL ~true -> -2
  a = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_not (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == -2);

  // BOOL ~false -> -1
  a = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_not (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == -1);

  // Unsupported types
  literal invalids[] = {
    { .type = LT_DECIMAL },
    { .type = LT_COMPLEX },
    { .type = LT_STRING },
    { .type = LT_ARRAY },
    { .type = LT_OBJECT },
  };

  for (u32 i = 0; i < sizeof (invalids) / sizeof (invalids[0]); i++)
    {
      a = invalids[i];
      err_t ret = literal_not (&a, &err);
      if (ret != ERR_SYNTAX)
        {
          i_log_failure ("Expected not to return syntax error: %s\n", literal_t_tostr (a.type));
        }
      test_assert (ret == ERR_SYNTAX);
      err.cause_code = SUCCESS;
    }
}

err_t
literal_minus (literal *dest, error *e)
{
  switch (dest->type)
    {
    case LT_COMPLEX:
      {
        dest->cplx = -dest->cplx;
        return SUCCESS;
      }
    case LT_INTEGER:
      {
        dest->integer = -dest->integer;
        return SUCCESS;
      }
    case LT_DECIMAL:
      {
        dest->decimal = -dest->decimal;
        return SUCCESS;
      }
    default:
      {
        break;
      }
    }

  return ERR_UNSUPPORTED_UN_OP (dest, "-");
}

TEST (literal_minus)
{
  error err = error_create (NULL);

  // -Integer
  literal a = { .type = LT_INTEGER, .integer = 5 };
  test_assert (literal_minus (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == -5);

  // -(-Integer)
  a = (literal){ .type = LT_INTEGER, .integer = -3 };
  test_assert (literal_minus (&a, &err) == SUCCESS);
  test_assert (a.type == LT_INTEGER);
  test_assert (a.integer == 3);

  // -Decimal
  a = (literal){ .type = LT_DECIMAL, .decimal = 2.5 };
  test_assert (literal_minus (&a, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (a.decimal == -2.5);

  // -(-Decimal)
  a = (literal){ .type = LT_DECIMAL, .decimal = -1.25 };
  test_assert (literal_minus (&a, &err) == SUCCESS);
  test_assert (a.type == LT_DECIMAL);
  test_assert (a.decimal == 1.25);

  // -Complex
  a = (literal){ .type = LT_COMPLEX, .cplx = 2.0 + 3.0 * I };
  test_assert (literal_minus (&a, &err) == SUCCESS);
  test_assert (a.type == LT_COMPLEX);
  test_assert (i_creal_64 (a.cplx) == -2.0);
  test_assert (i_cimag_64 (a.cplx) == -3.0);

  // -True (should fail)
  a = (literal){ .type = LT_BOOL, .bl = true };
  test_assert (literal_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -False (should fail)
  a = (literal){ .type = LT_BOOL, .bl = false };
  test_assert (literal_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -String (should fail)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("bad") };
  test_assert (literal_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -Array (should fail)
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 0 } };
  test_assert (literal_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;

  // -Object (should fail)
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 0 } };
  test_assert (literal_minus (&a, &err) == ERR_SYNTAX);
  err.cause_code = SUCCESS;
}

void
literal_bang (literal *dest)
{
  dest->bl = !literal_truthy (dest);
  dest->type = LT_BOOL;
}

TEST (literal_bang)
{
  // Integer (non-zero -> false)
  literal a = { .type = LT_INTEGER, .integer = 42 };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Integer (zero -> true)
  a = (literal){ .type = LT_INTEGER, .integer = 0 };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Decimal (non-zero -> false)
  a = (literal){ .type = LT_DECIMAL, .decimal = 42.0 };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Decimal (zero -> true)
  a = (literal){ .type = LT_DECIMAL, .decimal = 0.0 };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Complex (non-zero -> false)
  a = (literal){ .type = LT_COMPLEX, .cplx = 3.0 + 4.0 * I };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Complex (zero -> true)
  a = (literal){ .type = LT_COMPLEX, .cplx = 0.0 + 0.0 * I };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // True -> false
  a = (literal){ .type = LT_BOOL, .bl = true };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // False -> true
  a = (literal){ .type = LT_BOOL, .bl = false };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // String (non-empty -> false)
  a = (literal){ .type = LT_STRING, .str = unsafe_cstrfrom ("hello") };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // String (empty -> true)
  a = (literal){ .type = LT_STRING, .str = { .data = "", .len = 0 } };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Array (non-empty -> false)
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 1 } };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Array (empty -> true)
  a = (literal){ .type = LT_ARRAY, .arr = { .len = 0 } };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);

  // Object (non-empty -> false)
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 2 } };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == false);

  // Object (empty -> true)
  a = (literal){ .type = LT_OBJECT, .obj = { .len = 0 } };
  literal_bang (&a);
  test_assert (a.type == LT_BOOL);
  test_assert (a.bl == true);
}

void
i_log_literal (literal *v)
{
  switch (v->type)
    {
    case LT_OBJECT:
      {
        i_log_info ("====== OBJECT: \n");
        for (u32 i = 0; i < v->obj.len; ++i)
          {
            i_log_info ("Key: %.*s. Value: \n", v->obj.keys[i].len, v->obj.keys[i].data);
            i_log_literal (&v->obj.literals[i]);
          }
        i_log_info ("====== DONE \n");
        return;
      }
    case LT_ARRAY:
      {
        i_log_info ("====== ARRAY: \n");
        for (u32 i = 0; i < v->arr.len; ++i)
          {
            i_log_literal (&v->arr.literals[i]);
          }
        i_log_info ("====== DONE \n");
        return;
      }
    case LT_STRING:
      {
        i_log_info ("%.*s\n", v->str.len, v->str.data);
        return;
      }
    case LT_INTEGER:
      {
        i_log_info ("%d\n", v->integer);
        return;
      }
    case LT_DECIMAL:
      {
        i_log_info ("%f\n", v->decimal);
        return;
      }
    case LT_COMPLEX:
      {
        i_log_info ("%f %f\n", i_creal_64 (v->cplx), i_cimag_64 (v->cplx));
        return;
      }
    case LT_BOOL:
      {
        if (v->bl)
          {
            i_log_info ("TRUE\n");
          }
        else
          {
            i_log_info ("FALSE\n");
          }
        return;
      }
    }
}
