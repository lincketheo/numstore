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
} struct_t;

err_t struct_t_validate (const struct_t *t, error *e);
i32 struct_t_snprintf (char *str, u32 size, const struct_t *st);
u32 struct_t_byte_size (const struct_t *t);
u32 struct_t_get_serial_size (const struct_t *t);
void struct_t_serialize (serializer *dest, const struct_t *src);
err_t struct_t_deserialize (struct_t *dest, deserializer *src, lalloc *a, error *e);
err_t struct_t_random (struct_t *st, lalloc *alloc, u32 depth, error *e);
