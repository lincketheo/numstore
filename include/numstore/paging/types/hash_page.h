#pragma once

#include "core/errors/error.h" // err_t
#include "core/intf/types.h"   // pgno
#include "numstore/config.h"   // PAGE_SIZE

/**
 * HASH_PAGE
 * ============ PAGE START
 * HEADER
 * LEN
 * HASH0
 * HASH1
 * ...
 * HASH(LEN - 1)
 * 0              // Might be buffer if not multiple of sizeof(pgno)
 * 0
 * ============ PAGE END
 */
typedef struct
{
  pgh *header;  // Page header
  pgno *hashes; // Hashes pointing to linked list
} hash_page;

#define HP_HEDR_OFST ((p_size)0)
#define HP_HASH_OFST ((p_size)(HP_HEDR_OFST + sizeof (pgno)))

_Static_assert(PAGE_SIZE > HP_HASH_OFST + 10 * sizeof (hash_page),
               "Hash Page: PAGE_SIZE must be > HP_HASH_OFST "
               "plus at least 10 extra hashes");
#define HP_NHASHES ((p_size)((PAGE_SIZE - HP_HASH_OFST) / sizeof (pgno)))
_Static_assert(HP_NHASHES > 10, "Hash Page: HP_NHASHES must be > 10");

err_t hp_validate (const hash_page *d, error *e);
void hp_init_empty (hash_page *hp);
void hp_set_ptrs (hash_page *hp, u8 raw[PAGE_SIZE]);
p_size hp_get_hash_pos (const hash_page *p, const string str);
pgno hp_get_pgno (const hash_page *p, p_size pos);
void hp_set_hash (hash_page *p, p_size pos, pgno pg);
void i_log_hp (const hash_page *hp);
