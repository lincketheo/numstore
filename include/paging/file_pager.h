#pragma once

#include "intf/io.h"
#include "intf/types.h"

typedef struct
{
  p_size page_size;
  p_size header_size;
  pgno npages;
  i_file f;
} file_pager;

typedef struct
{
  i_file f;
  p_size header_size;
  p_size page_size;
} fpgr_params;

/**
 * Errors:
 *    - ERR_IO - fstat failure
 *    - ERR_CORRUPT - If the file is found in a bad spot on initial open
 */
err_t fpgr_create (file_pager *dest, fpgr_params p, error *e);

/**
 * Errors:
 *    - ERR_IO - ftruncate failure
 */
err_t fpgr_new (file_pager *p, pgno *pgno_dest, error *e);

/**
 * Error:
 *    - ERR_IO - pread fail
 *    - ERR_CORRUPT - empty read - (page doesn't exist)
 */
err_t fpgr_get_expect (file_pager *p, u8 *dest, pgno pgno, error *e);

/**
 * Doesn't do anything yet
 */
err_t fpgr_delete (file_pager *p, pgno pgno, error *e);

/**
 * Errors:
 *    - ERR_IO - pwrite error on page write
 */
err_t fpgr_commit (file_pager *p, const u8 *src, pgno pgno, error *e);
