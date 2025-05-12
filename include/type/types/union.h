#pragma once

#include "ds/strings.h"         // string
#include "errors/error.h"       // error
#include "intf/types.h"         // u32
#include "mm/lalloc.h"          // lalloc
#include "utils/deserializer.h" // deserializer
#include "utils/serializer.h"   // serializer

typedef struct type_s type;

typedef struct
{
  u16 len;
  string *keys;
  type *types;
} union_t;

err_t union_t_validate (const union_t *t, error *e);

int union_t_snprintf (char *str, u32 size, const union_t *p);

u32 union_t_byte_size (const union_t *t);

u32 union_t_get_serial_size (const union_t *t);

void union_t_serialize (serializer *dest, const union_t *src);

err_t union_t_deserialize (union_t *dest, deserializer *src, lalloc *a, error *e);
