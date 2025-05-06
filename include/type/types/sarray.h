#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

typedef struct type_s type;

typedef struct
{
  u16 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;

bool sarray_t_is_valid (const sarray_t *t);

int sarray_t_snprintf (char *str, u32 size, const sarray_t *p);

u32 sarray_t_byte_size (const sarray_t *t);

void sarray_t_free_internals_forgiving (sarray_t *t, lalloc *alloc);

void sarray_t_free_internals (sarray_t *t, lalloc *alloc);

u32 sarray_t_get_serial_size (const sarray_t *t);

void sarray_t_serialize (serializer *dest, const sarray_t *src);

err_t sarray_t_deserialize (sarray_t *dest, deserializer *src, lalloc *a);
