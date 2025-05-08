#pragma once

#include "dev/assert.h"
#include "ds/strings.h"
#include "type/types.h"

/**
 * Variable Models and transformations from
 * 1 model to another
 */

typedef struct
{
  pgno pgn0; // Root page number
  type type; // Variable Type
} vmeta;

typedef struct
{
  char *vstr;
  u16 vlen;
  u8 *tstr;
  u16 tlen;
  pgno pg0; // The starting page
} var_hash_entry;

err_t vm_to_vhe (
    var_hash_entry *dest,
    string vname,
    vmeta src,
    lalloc *tstr_allocator);

void var_hash_entry_free (var_hash_entry *v, lalloc *tstr_allocator);

err_t vhe_to_vm (
    vmeta *dmeta,
    const var_hash_entry *src,
    lalloc *type_allocator);
