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

/**
 * Errors:
 *   - ERR_NOMEM - if tstr_allocator can't fit the tstr
 */
err_t vm_to_vhe (
    var_hash_entry *dest,   // The var hash entry we want to convert into
    string vname,           // The variable name
    vmeta src,              // The meta information we want to convert
    lalloc *tstr_allocator, // The allocator for the type string
    error *e                // Error accumulator
);

/**
 * Free's the contents of [v] given it was allocated on [tstr_allocator]
 */
void var_hash_entry_free (var_hash_entry *v, lalloc *tstr_allocator);

/**
 * Errors:
 *   - ERR_NOMEM - Can't fit type in type_allocator
 *   - ERR_INVALID_TYPE - Deserialized type, but result was invalid
 *   - ERR_TYPE_DESER - Failed to deserialize type
 */
err_t vhe_to_vm (
    vmeta *dmeta,              // The destination meta
    const var_hash_entry *src, // The source data to convert
    lalloc *type_allocator,    // The allocator for the resulting type
    error *e                   // Error accumulator
);
