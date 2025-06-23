#pragma once

#include "paging/pager.h"
#include "variables/variable.h"

#define ISEOF (1 << 0)
#define ISPREVTOMB (1 << 1)

typedef struct hl_s hl;

hl *hl_open (pager *p, error *e);
void hl_free (hl *v);

bool hl_eof (hl *h);

err_t hl_seek (hl *h, var_hash_entry *dest, pgno pg, error *e);
err_t hl_read (hl *h, var_hash_entry *dest, error *e);
void hl_set_tombstone (hl *h);
err_t hl_append (hl *h, const variable v, error *e);
