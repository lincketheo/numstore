#pragma once

#include "core/errors/error.h" // err_t
#include "core/intf/io.h"      // i_file
#include "core/intf/types.h"   // pgno

#include "numstore/config.h" // PAGE_SIZE

typedef struct file_pager_s file_pager;

/**
 * Opens [fname] as a file pager, or creates a new file with size = 0
 *
 * Errors:
 *    - ERR_IO - fstat failure
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
file_pager *fpgr_open (const string fname, error *e);
err_t fpgr_close (file_pager *f, error *e);

p_size fpgr_get_npages (const file_pager *fp);

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
err_t fpgr_read (file_pager *p, u8 dest[PAGE_SIZE], pgno pgno, error *e);

/**
 * Writes a page back to disk
 *
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t fpgr_write (file_pager *p, const u8 src[PAGE_SIZE], pgno pgno, error *e);

/**
 * Deletes a page
 *
 * But it doesn't do anything yet
 */
err_t fpgr_delete (file_pager *p, pgno pgno, error *e);
