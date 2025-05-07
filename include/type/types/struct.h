#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

typedef struct type_s type;

typedef struct
{
  u16 len;
  string *keys;
  type *types;
} struct_t;

bool struct_t_is_valid (const struct_t *t);

int struct_t_snprintf (char *str, u32 size, const struct_t *st);

u32 struct_t_byte_size (const struct_t *t);

u32 struct_t_get_serial_size (const struct_t *t);

void struct_t_serialize (serializer *dest, const struct_t *src);

err_t struct_t_deserialize (struct_t *dest, deserializer *src, salloc *a);
