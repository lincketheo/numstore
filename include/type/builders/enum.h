#pragma once

#include "type/types/enum.h"

typedef struct
{
  // The raw string data
  char *alldata; // One long string for all the keys
  u32 adlen;     // Not necessary - but peace of mind
  u32 adcap;     // capacity of alldata

  struct
  {
    u32 idx;   // Offset in alldata buffer
    u32 klen;  // Length of this string
  } * allstrs; // The pointers for all the data
  u32 aslen;   // len of allstrs (not adlen - that's total length)
  u32 ascap;   // capacity of allstrs

  lalloc *alloc;
} enum_builder;

err_t enb_create (enum_builder *dest, lalloc *alloc, error *e);
err_t enb_accept_key (enum_builder *eb, const string key, error *e);

/**
 * Both build and free will free data,
 * free should be used if build was unsuccessful
 * or not used
 */
void enb_free (enum_builder *enb);
err_t enb_build (enum_t *dest, enum_builder *eb, error *e);
