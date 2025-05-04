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
  p_size *len;  // Length of the hash table
  pgno *hashes; // Hashes pointing to linked list
} hash_page;

err_t hp_get_hash (page *p, pgno *dest, const string string);
