#pragma once

#include "core/intf/types.h"
#include "core/mm/lalloc.h"
#include "numstore/paging/pager.h"
#include "numstore/paging/types/hash_leaf.h"
#include "numstore/rptree/internal/seek.h"

typedef struct rptree_s rptree;

/**
 * If [pg0] is -1, creates a new rptree
 *
 * Errors:
 *   - From pgr_new:
 *    - ERR_CORRUPT
 *    - ERR_IO
 */
rptree *rpt_open (spgno pg0, pager *p, error *e);
void rptree_close (rptree *r);

/**
 * Seeks to byte b
 *
 * On any subsequent operation, if [r] is unseeked,
 * it will call seek(0) first.
 */
err_t rpt_seek (rptree *r, b_size b, error *e);

/**
 * Returns the current position of [r]
 */
b_size rpt_tell (rptree *r);

/**
 * Checks if [r] is at the end of the data
 */
bool rpt_eof (rptree *r);

/**
 * Returns the starting page of [r]
 */
pgno rpt_pg0 (rptree *r);

/**
 * Reads maximum of [n] elements into [dest], each element
 * of size [size] (so [dest] needs to be at least n * size
 * bytes long) while skipping [nskip] elements every read
 *
 * Returns ERR (< 0) on failure or number of elements read
 */
sb_size rpt_read (
    u8 *dest, t_size size, b_size n, b_size nskip,
    rptree *r, error *e);

/**
 * Scans the exact same way as read, but
 * deletes element at each location it hits
 *
 * if [dest] is present, writes deleted data to dest
 */
sb_size rpt_delete (
    u8 *dest, t_size size, b_size n, b_size nskip,
    rptree *r, error *e);

/**
 * Inserts data at the current seek position
 *
 * Errors:
 *   - From pgr_get_expect
 *      - ERR_CORRUPT - Expects inner nodes and data lists
 */
sb_size rpt_insert (
    const u8 *src, t_size size, b_size n,
    rptree *r, error *e);

/**
 * (Over) writes data at the current seek position
 *
 * If [nskip]
 *
 * Errors:
 *   - From pgr_get_expect
 *      - ERR_CORRUPT - Expects inner nodes and data lists
 */
sb_size rpt_write (
    const u8 *src, t_size size, b_size n, b_size nskip,
    rptree *r, error *e);
