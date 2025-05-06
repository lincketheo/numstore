#pragma once

#include "dev/errors.h"
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
} union_t;

bool union_t_is_valid (const union_t *t);

int union_t_snprintf (char *str, u32 size, const union_t *p);

u32 union_t_byte_size (const union_t *t);

void union_t_free_internals_forgiving (union_t *t, lalloc *alloc);

void union_t_free_internals (union_t *t, lalloc *alloc);

u32 union_t_get_serial_size (const union_t *t);

void union_t_serialize (serializer *dest, const union_t *src);

err_t union_t_deserialize (union_t *dest, deserializer *src, lalloc *a);
