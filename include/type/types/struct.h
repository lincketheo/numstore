#pragma once

#include "ds/strings.h"         // string
#include "errors/error.h"       // error
#include "intf/mm.h"            // lalloc
#include "intf/types.h"         // u32
#include "utils/deserializer.h" // deserializer
#include "utils/serializer.h"   // serializer

typedef struct type_s type;

typedef struct
{
  u16 len;
  string *keys;
  type *types;

  u8 *data;
} struct_t;

err_t struct_t_validate (const struct_t *t, error *e);

int struct_t_snprintf (char *str, u32 size, const struct_t *st);

u32 struct_t_byte_size (const struct_t *t);

void struct_t_free_internals_forgiving (struct_t *t, lalloc *alloc);

void struct_t_free_internals (struct_t *t, lalloc *alloc);

u32 struct_t_get_serial_size (const struct_t *t);

void struct_t_serialize (serializer *dest, const struct_t *src);

err_t struct_t_deserialize (struct_t *dest, deserializer *src, lalloc *a, error *e);
