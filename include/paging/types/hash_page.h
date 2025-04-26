#pragma once

#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/types.h"

typedef struct page_s page;

/**
 * HASH_PAGE
 * ============ PAGE START
 * HEADER
 * LEN
 * HASH0
 * HASH1
 * ...
 * HASH(LEN - 1)
 * 0
 * 0
 * ============ PAGE END
 */
typedef struct
{
  u32 *len;    // Length of the hash table
  u64 *hashes; // Hashes pointing to header_size of linked list
} hash_page;

err_t hp_get_hash (u64 *dest, page *p, const string string);
