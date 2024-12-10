#include "ns_datasource.h"

#include "ns_buffer.h"
#include "ns_errors.h"
#include "ns_macros.h"
#include "ns_memory.h"

/////////////////////////////////
/// In memory data source

PRIVATE ns_ret_t
mem_datasource_close (ns_datasource *d)
{
  ASSERT (d);
  mem_datasource *m = &d->mdatasource;
  bytes b = buftobytes (m->memory);
  return m->a->ns_free (m->a, &b);
}

PRIVATE ns_ret_t
mem_datasource_read (ns_datasource *d, buf *dest, ssrange s)
{
  ASSERT (d);
  mem_datasource *m = &d->mdatasource;
  buf_move_buf (&m->memory, dest, s);
  return NS_OK;
}

PRIVATE ns_ret_t
mem_datasource_write_move (ns_datasource *d, buf *src, ssrange s)
{
  ASSERT (d);
  mem_datasource *m = &d->mdatasource;
  buf_move_buf (src, &m->memory, s);
  return NS_OK;
}

PRIVATE ns_ret_t
mem_datasource_append_move (ns_datasource *d, buf *src, ssrange s)
{
  ASSERT (d);
  mem_datasource *m = &d->mdatasource;
  buf_move_buf (src, &m->memory, s);
  return NS_OK;
}

PRIVATE inline void
mem_datasource_set_methods (ns_datasource *d)
{
  ASSERT (d);
  d->ns_datasource_close = mem_datasource_close;
  d->ns_datasource_read = mem_datasource_read;
}

ns_ret_t
mem_datasource_create_alloc (ns_datasource *dest, ns_alloc *a, ns_size cap)
{
  ASSERT (dest);
  ASSERT (a);
  ASSERT (cap > 0);

  bytes data = bytes_create_empty ();
  ns_ret_t ret = a->ns_malloc (a, &data, cap);
  if (ret != NS_OK)
    {
      return ret;
    }

  buf memory = buf_create_empty_from_bytes (data, 1);

  dest->mdatasource = (mem_datasource){
    .memory = memory,
    .a = a,
  };

  return NS_OK;
}
