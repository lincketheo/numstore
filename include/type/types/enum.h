#pragma once

#include "ds/strings.h"         // string
#include "errors/error.h"       // error
#include "intf/types.h"         // u32
#include "mm/lalloc.h"          // lalloc
#include "utils/deserializer.h" // deserializer
#include "utils/serializer.h"   // serializer

typedef struct
{
  u16 len;
  string *keys;
} enum_t;

err_t enum_t_validate (const enum_t *t, error *e);

int enum_t_snprintf (char *str, u32 size, const enum_t *st);

#define enum_t_byte_size(e) sizeof (u8)

u32 enum_t_get_serial_size (const enum_t *t);

void enum_t_serialize (serializer *dest, const enum_t *src);

err_t enum_t_deserialize (enum_t *dest, deserializer *src, lalloc *a, error *e);
