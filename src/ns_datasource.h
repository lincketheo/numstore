#pragma once

#include "ns_buffer.h"
#include "ns_errors.h"
#include "ns_memory.h"
#include "ns_srange.h"
#include "ns_types.h"

/////////////////////////////////
/// In memory data source

/**
 * An in memory data source
 */
typedef struct
{
  buf memory;
  ns_alloc *a;
} mem_datasource;

/////////////////////////////////
/// Abstraction

typedef struct ns_datasource_s ns_datasource;

/**
 * A data source is just a contiguous array of bytes
 * represented persistently in some way
 *
 * Opening a data source is data source specific
 *
 * All methods in ns_datasource are common
 *
 *
 */
struct ns_datasource_s
{
  union
  {
    mem_datasource mdatasource;
  };

  ns_ret_t (*ns_datasource_close) (ns_datasource *d);
  ns_ret_t (*ns_datasource_read) (ns_datasource *d, buf *dest, ssrange s);
  ns_ret_t (*ns_datasource_write_move) (ns_datasource *d, buf *src, srange s);
  ns_ret_t (*ns_datasource_append_move) (ns_datasource *d, buf *src, srange s);
};

ns_ret_t mem_datasource_create_alloc (ns_datasource *dest, ns_alloc *a, ns_size cap);
