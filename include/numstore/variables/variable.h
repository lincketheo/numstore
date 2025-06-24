#pragma once

#include "core/ds/strings.h" // TODO

#include "compiler/ast/type/types.h" // TODO

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
  string vstr;       // Variable name
  u8 *tstr;          // Serialized type string
  u16 tlen;          // Length of [tstr]
  pgno pg0;          // The starting page
  bool is_tombstone; // Is this var deleted
} var_hash_entry;

/**
 * Converts a [variable] into a [var_hash_entry]
 * by serializing the type string
 *
 * Errors:
 *   - ERR_NOMEM - if [alloc] can't fit tstr
 *
 * Allocations:
 *   - Allocates the serialized type string
 *   - Allocates memory for the string variable name
 */
err_t var_hash_entry_create (
    var_hash_entry *dest, // The var hash entry we want to convert into
    const variable *src,  // The variable to convert
    lalloc *alloc,        // The allocator for the strings
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
 *   - Copy string over
 */
err_t var_hash_entry_deserialize (
    variable *dest,            // Destination variable to write into
    const var_hash_entry *src, // The source data to convert
    lalloc *alloc,             // The allocator for the resulting type
    error *e                   // Error accumulator
);
