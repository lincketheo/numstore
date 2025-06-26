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
} struct_t;

err_t struct_t_validate (const struct_t *t, error *e);
i32 struct_t_snprintf (char *str, u32 size, const struct_t *st);
u32 struct_t_byte_size (const struct_t *t);
u32 struct_t_get_serial_size (const struct_t *t);
void struct_t_serialize (serializer *dest, const struct_t *src);
err_t struct_t_deserialize (struct_t *dest, deserializer *src, lalloc *a, error *e);
err_t struct_t_random (struct_t *st, lalloc *alloc, u32 depth, error *e);
bool struct_t_equal (const struct_t *left, const struct_t *right);
