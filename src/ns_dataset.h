#pragma once

#include "ns_buffer.h"
#include "ns_dsrc.h"
#include "ns_dtype.h"
#include "ns_types.h"

typedef struct
{
  ns_dsrc data;
} ns_ds;

typedef struct
{
  dtype type;
  ns_size nelem;
} ns_ds_header;

ns_ret_t ns_ds_close (ns_ds *d);
ns_ret_t ns_ds_read (ns_ds *d, buf* dest, ssrange s);
ns_ret_t ns_ds_write_move (ns_ds *d, buf* src, ssrange s);
