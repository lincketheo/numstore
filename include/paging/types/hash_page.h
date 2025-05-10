#pragma once

#include "ds/strings.h"
#include "errors/error.h"
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
  u8 *raw;
  p_size rlen;

  pgh *header;
  pgno *hashes; // Hashes pointing to linked list
} hash_page;

err_t hp_validate (const hash_page *d, error *e);

void hp_init_empty (hash_page *hp);

hash_page hp_set_ptrs (u8 *raw, p_size len);

p_size hp_hash_len (p_size page_size);

p_size hp_get_hash_pos (const hash_page *p, const string str);

pgno hp_get_pgno (const hash_page *p, p_size pos);

void hp_set_hash (hash_page *p, p_size pos, pgno pg);
