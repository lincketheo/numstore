#include "compiler/value/builders/array.h"
#include "core/dev/assert.h"
#include "core/ds/llist.h"
#include "core/errors/error.h"

DEFINE_DBG_ASSERT_I (array_builder, array_builder, a)
{
  ASSERT (a);
}

static const char *TAG = "Array Value Builder";

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
arb_accept_value (array_builder *o, value v, error *e)
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
          return error_causef (e, ERR_NOMEM, "%s: allocation failed", TAG);
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

  value *values = lmalloc (b->dest, length, sizeof *values);
  if (!values)
    {
      return error_causef (e, ERR_NOMEM, "%s: allocation failed", TAG);
    }

  u16 i = 0;
  for (llnode *it = b->head; it; it = it->next)
    {
      array_llnode *dn = container_of (it, array_llnode, link);
      values[i++] = dn->v;
    }

  dest->len = length;
  dest->values = values;

  return SUCCESS;
}
