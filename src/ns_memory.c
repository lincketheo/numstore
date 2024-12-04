#include "ns_memory.h"
#include "ns_errors.h"

static char quick_heap[2048];

size_t
quick_heap_start ()
{
  return 0;
}

bool
quick_heap_avail (size_t ctx, size_t bytes)
{
  return (ctx + bytes) < sizeof (quick_heap);
}

void *
quick_heap_malloc (size_t *ctx, size_t bytes)
{
  ASSERT (quick_heap_avail (*ctx, bytes) && "Quick Heap Overflow");
  void *ret = &quick_heap[*ctx];
  *ctx += bytes;
  return ret;
}

linalloc
linalloc_create_qh ()
{
  return (linalloc){
    .data = quick_heap,
    .cap = sizeof (quick_heap),
    .len = 0,
  };
}

int
linalloc_avail (linalloc *l, size_t bytes)
{
  ASSERT (l);
  return l->len + bytes <= l->cap;
}

void *
linalloc_malloc (linalloc *l, size_t bytes)
{
  ASSERT (linalloc_avail (l, bytes));
  void *ret = &((char *)l->data)[l->len];
  l->len += bytes;
  return ret;
}

void
linalloc_free (linalloc *l, void *ptr)
{
  return;
}

int
ns_alloc_avail (ns_alloc *a, size_t bytes)
{
  ASSERT (a);

  switch (a->type)
    {
    case LINEAR:
      return linalloc_avail (&a->l, bytes);
    default:
      unreachable ();
    }
}

ns_alloc
ns_alloc_create_qh (ns_alloc_type type)
{
  switch (type)
    {
    case LINEAR:
      return (ns_alloc){
        .l = linalloc_create_qh (),
        .type = type,
      };
    default:
      unreachable ();
    }
}

void *
ns_alloc_malloc (ns_alloc *a, size_t bytes)
{
  ASSERT (a);

  switch (a->type)
    {
    case LINEAR:
      return linalloc_malloc (&a->l, bytes);
    default:
      unreachable ();
    }
}

void
ns_alloc_free (ns_alloc *a, void *ptr)
{
  ASSERT (a);
  ASSERT (ptr);

  switch (a->type)
    {
    case LINEAR:
      linalloc_free (&a->l, ptr);
      break;
    default:
      unreachable ();
    }
}
