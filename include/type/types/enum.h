#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

typedef struct
{
  u16 len;
  string *keys;
} enum_t;

bool enum_t_is_valid (const enum_t *t);

int enum_t_snprintf (char *str, u32 size, const enum_t *st);

u32 enum_t_byte_size (const enum_t *t);

u32 enum_t_get_serial_size (const enum_t *t);

void enum_t_serialize (serializer *dest, const enum_t *src);

err_t enum_t_deserialize (enum_t *dest, deserializer *src, salloc *a);
