#pragma once

#include "core/ds/strings.h"         // string
#include "core/errors/error.h"       // error
#include "core/intf/types.h"         // u32
#include "core/mm/lalloc.h"          // lalloc
#include "core/utils/deserializer.h" // deserializer
#include "core/utils/serializer.h"   // serializer

typedef struct type_s type;

typedef struct
{
  u16 len;
  string *keys;
  type *types;
} union_t;

err_t union_t_validate (const union_t *t, error *e);
i32 union_t_snprintf (char *str, u32 size, const union_t *p);
u32 union_t_byte_size (const union_t *t);
u32 union_t_get_serial_size (const union_t *t);
void union_t_serialize (serializer *dest, const union_t *src);
err_t union_t_deserialize (union_t *dest, deserializer *src, lalloc *a, error *e);
err_t union_t_random (union_t *un, lalloc *alloc, u32 depth, error *e);
bool union_t_equal (const union_t *left, const union_t *right);
