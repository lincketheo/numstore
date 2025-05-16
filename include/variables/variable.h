#pragma once

#include "ast/type/types.h"
#include "ds/strings.h"

/**
 * The domain layer variable data structure
 */
typedef struct
{
  string vname;
  type type;
  pgno pg0;
} variable;

/**
 * The hash table representation of a variable
 * Serialized and everything
 */
typedef struct
{
  char *vstr; // Variable name
  u8 *tstr;   // Serialized type string
  u16 vlen;   // Length of [vstr]
  u16 tlen;   // Length of [tstr]
  pgno pg0;   // The starting page
} var_hash_entry;

/**
 * Errors:
 *   - ERR_NOMEM - if [alloc] can't fit tstr
 *
 * Allocations:
 *   - Allocates the serialized type string
 */
err_t var_hash_entry_create (
    var_hash_entry *dest, // The var hash entry we want to convert into
    const variable *src,  // The variable to convert
    lalloc *alloc,        // The allocator for the type string
    error *e              // Error accumulator
);

/**
 * Errors:
 *   - ERR_NOMEM - Can't fit type in type_allocator
 *   - ERR_INVALID_TYPE - Deserialized type, but result was invalid
 *   - ERR_TYPE_DESER - Failed to deserialize type
 *
 * Allocations:
 *   - Type data structures from [type_deserialize]
 *     Doesn't copy strings
 */
err_t var_hash_entry_deserialize (
    variable *dest,            // Destination variable to write into
    const var_hash_entry *src, // The source data to convert
    lalloc *alloc,             // The allocator for the resulting type
    error *e                   // Error accumulator
);
