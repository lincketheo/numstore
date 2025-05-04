#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"

typedef struct type_s type;

typedef struct
{
  u16 len;
  string *keys;
} enum_t;

void enum_free (type *t, lalloc *alloc);
err_t ser_size_enum (u16 *dest, const type *t);
void i_log_enum (const type *t);
err_t enum_t_deserialize (type *dest, lalloc *a, const u8 *src, u16 slen);
err_t enum_t_serialize (u8 *t, u16 dlen, const type *src);
