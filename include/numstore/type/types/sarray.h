#pragma once

#include "core/errors/error.h"       // error
#include "core/intf/types.h"         // u32
#include "core/mm/lalloc.h"          // lalloc
#include "core/utils/deserializer.h" // deserializer
#include "core/utils/serializer.h"   // serializer

typedef struct type_s type;

typedef struct
{
  u16 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;

err_t sarray_t_validate (const sarray_t *t, error *e);
i32 sarray_t_snprintf (char *str, u32 size, const sarray_t *p);
u32 sarray_t_byte_size (const sarray_t *t);
u32 sarray_t_get_serial_size (const sarray_t *t);
void sarray_t_serialize (serializer *dest, const sarray_t *src);
err_t sarray_t_deserialize (sarray_t *dest, deserializer *src, lalloc *a, error *e);
err_t sarray_t_random (sarray_t *sa, lalloc *alloc, u32 depth, error *e);
bool sarray_t_equal (const sarray_t *left, const sarray_t *right);
