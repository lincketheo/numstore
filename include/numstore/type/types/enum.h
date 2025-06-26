#pragma once

#include "core/ds/strings.h"         // string
#include "core/errors/error.h"       // error
#include "core/intf/types.h"         // u32
#include "core/mm/lalloc.h"          // lalloc
#include "core/utils/deserializer.h" // deserializer
#include "core/utils/serializer.h"   // serializer

typedef struct
{
  u16 len;
  string *keys;
} enum_t;

err_t enum_t_validate (const enum_t *t, error *e);
i32 enum_t_snprintf (char *str, u32 size, const enum_t *st);
#define enum_t_byte_size(e) sizeof (u8)
u32 enum_t_get_serial_size (const enum_t *t);
void enum_t_serialize (serializer *dest, const enum_t *src);
err_t enum_t_deserialize (enum_t *dest, deserializer *src, lalloc *a, error *e);
err_t enum_t_random (enum_t *en, lalloc *alloc, error *e);
bool enum_t_equal (const enum_t *left, const enum_t *right);
