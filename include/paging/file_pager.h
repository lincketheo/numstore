#pragma once

#include "config.h"       // PAGE_SIZE
#include "errors/error.h" // err_t
#include "intf/io.h"      // i_file
#include "intf/types.h"   // pgno

typedef struct
{
  pgno npages; // Cached so you don't need to call fsize a ton
  i_file f;    // The file we're working with
} file_pager;

/**
 * Opens [fname] as a file pager, or creates a new file with size = 0
 *
 * Errors:
 *    - ERR_IO - fstat failure
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
err_t fpgr_create (file_pager *dest, const string fname, error *e);
void fpgr_close (file_pager *f);

/**
 * Creates a new page and stores the result in [pgno_dest]
 *
 * Errors:
 *    - ERR_IO - ftruncate failure
 */
err_t fpgr_new (file_pager *p, pgno *pgno_dest, error *e);

/**
 * Gets a page that it expects to be there, or else returns
 * an ERR_CORRUPT error
 *
 * Error:
 *    - ERR_IO - pread fail
 *    - ERR_CORRUPT - empty read - (page doesn't exist)
 */
err_t fpgr_get_expect (
    file_pager *p,
    u8 dest[PAGE_SIZE],
    pgno pgno,
    error *e);

/**
 * Doesn't do anything yet
 */
err_t fpgr_delete (file_pager *p, pgno pgno, error *e);

/**
 * Writes a page back to disk
 *
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t fpgr_write (
    file_pager *p,
    const u8 src[PAGE_SIZE],
    pgno pgno,
    error *e);
