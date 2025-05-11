#pragma once

#include "ds/strings.h"         // string
#include "errors/error.h"       // error
#include "intf/mm.h"            // lalloc
#include "intf/types.h"         // u32
#include "utils/deserializer.h" // deserializer
#include "utils/serializer.h"   // serializer

typedef struct
{
  u16 len;

  /**
   * 2 allocs:
   */
  string *keys; // Points into [data]
  char *data;   // Backing data for keys
} enum_t;

err_t enum_t_validate (const enum_t *t, error *e);

int enum_t_snprintf (char *str, u32 size, const enum_t *st);

#define enum_t_byte_size(e) sizeof (u8)

void enum_t_free_internals_forgiving (enum_t *t, lalloc *alloc);

void enum_t_free_internals (enum_t *t, lalloc *alloc);

u32 enum_t_get_serial_size (const enum_t *t);

void enum_t_serialize (serializer *dest, const enum_t *src);

err_t enum_t_deserialize (enum_t *dest, deserializer *src, lalloc *a, error *e);
