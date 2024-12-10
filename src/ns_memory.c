#include "ns_memory.h"
#include "ns_bytes.h"
#include "ns_errors.h"
#include "ns_logging.h"
#include "ns_macros.h"
#include "ns_types.h"

#include <errno.h>  // errno
#include <stdlib.h> // malloc
#include <string.h> // strerror

///////////////////////////////
////////// Standard library facade

static volatile ns_size num_allocs = 0;

PRIVATE inline ns_ret_t
ns_malloc_facade (bytes *dest, ns_size _bytes)
{
  ASSERT (dest);
  ASSERT (_bytes > 0);
  ASSERT (bytes_IS_FREE (dest));

  errno = 0;
  ns_byte *ret = malloc (_bytes);

  if (ret == NULL)
    {
      ns_log (ERROR,
              "Failed to allocate %zu bytes "
              "of system memory\n",
              _bytes);
      return errno_to_ns_error (errno);
    }

  num_allocs++;
  *dest = bytes_create_from (ret, _bytes);
  return NS_OK;
}

PRIVATE inline ns_ret_t
ns_realloc_facade (bytes *p, ns_size _bytes)
{
  bytes_ASSERT (p);
  ASSERT (_bytes > 0);
  ASSERT (!bytes_IS_FREE (p));

  errno = 0;
  ns_byte *ret = realloc (p->data, _bytes);

  if (ret == NULL)
    {
      ns_log (ERROR,
              "Failed to reallocate memory from %zu bytes "
              "to %zu bytes "
              "in standard allocator. Error: %s\n",
              p->cap, _bytes, strerror (errno));
      return errno_to_ns_error (errno);
    }

  *p = bytes_update (*p, ret, _bytes);
  return NS_OK;
}

ns_ret_t
ns_free_facade (bytes *dest)
{
  bytes_ASSERT (dest);
  ASSERT (!bytes_IS_FREE (dest));
  ASSERT (num_allocs > 0);

  free (dest->data);
  num_allocs--;
  *dest = bytes_update (*dest, NULL, 0);

  return NS_OK;
}

///////////////////////////////
////////// Linalloc

PRIVATE inline ns_ret_t
__limited_linalloc_create (limited_linalloc *dest, ns_size cap)
{
  ASSERT (dest);
  ASSERT (cap > 0);

  bytes pool;
  ns_ret_t ret = ns_malloc_facade (&pool, cap);
  if (ret != NS_OK)
    {
      return ret;
    }

  limited_linalloc lret = {
    .pool = pool.data,
    .len = 0,
    .cap = pool.cap,
  };
  memcpy (dest, &ret, sizeof ret);

  return NS_OK;
}

PRIVATE inline ns_ret_t
__limited_linalloc_create_from (limited_linalloc *dest, ns_byte *pool,
                                ns_size cap)
{
  ASSERT (dest);
  ASSERT (cap > 0);

  limited_linalloc ret = {
    .pool = pool,
    .cap = cap,
    .len = 0,
  };
  memcpy (dest, &ret, sizeof ret);

  return NS_OK;
}

PRIVATE inline ns_bool
limited_linalloc_avail (limited_linalloc *l, ns_size bytes)
{
  ASSERT (l);
  return l->len + bytes <= l->cap;
}

PRIVATE ns_ret_t
limited_linalloc_malloc (ns_alloc *a, bytes *dest, ns_size _bytes)
{
  ASSERT (a);
  bytes_ASSERT (dest);
  ASSERT (bytes_IS_FREE (dest));
  ASSERT (_bytes > 0);

  limited_linalloc *l = &a->llalloc;

  if (!limited_linalloc_avail (l, _bytes))
    {
      return NS_NOMEM;
    }

  ns_byte *ret = &((ns_byte *)l->pool)[l->len];
  l->len += _bytes;

  ASSERT (l->len < l->cap);

  *dest = bytes_create_from (ret, _bytes);

  return NS_OK;
}

PRIVATE ns_ret_t
limited_linalloc_free (ns_alloc *l, bytes *p)
{
  ASSERT (l);
  bytes_ASSERT (p);
  ASSERT (!bytes_IS_FREE (p));
  return NS_UNSUPPORTED;
}

PRIVATE ns_ret_t
limited_linalloc_realloc (ns_alloc *l, bytes *p, size_t newsize)
{
  ASSERT (l);
  bytes_ASSERT (p);
  ASSERT (!bytes_IS_FREE (p));
  ASSERT (newsize > 0);
  return NS_UNSUPPORTED;
}

PRIVATE inline void
ns_alloc_set_linalloc_methods (ns_alloc *dest)
{
  ASSERT (dest);
  dest->ns_free = limited_linalloc_free;
  dest->ns_malloc = limited_linalloc_malloc;
  dest->ns_realloc = limited_linalloc_realloc;
}

ns_ret_t
ns_alloc_create_limited_linmem (ns_alloc *dest, ns_size cap)
{
  ASSERT (dest);
  ns_ret_t ret;

  if ((ret = __limited_linalloc_create (&dest->llalloc, cap)) != NS_OK)
    {
      return ret;
    }

  ns_alloc_set_linalloc_methods (dest);

  return NS_OK;
}

ns_ret_t
limited_linalloc_create_from (ns_alloc *dest, ns_byte *pool, ns_size cap)
{
  ASSERT (dest);
  ns_ret_t ret;

  if ((ret = __limited_linalloc_create_from (&dest->llalloc, pool, cap))
      != NS_OK)
    {
      return ret;
    }

  ns_alloc_set_linalloc_methods (dest);

  return NS_OK;
}

///////////////////////////////
////////// Standard

PRIVATE ns_ret_t
stdlib_malloc (ns_alloc *a, bytes *dest, ns_size bytes)
{
  return ns_malloc_facade (dest, bytes);
}

PRIVATE ns_ret_t
stdlib_free (ns_alloc *l, bytes *p)
{
  return ns_free_facade (p);
}

PRIVATE ns_ret_t
stdlib_realloc (ns_alloc *l, bytes *p, size_t newsize)
{
  return ns_realloc_facade (p, newsize);
}

PRIVATE inline void
ns_alloc_set_stdalloc_methods (ns_alloc *dest)
{
  ASSERT (dest);
  dest->ns_free = stdlib_free;
  dest->ns_malloc = stdlib_malloc;
  dest->ns_realloc = stdlib_realloc;
}

ns_ret_t
stdalloc_create (ns_alloc *dest)
{
  ASSERT (dest);
  ns_ret_t ret;
  ns_alloc_set_stdalloc_methods (dest);
  return NS_OK;
}
