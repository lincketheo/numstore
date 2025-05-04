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
 * Returns:
 *   - ERR_INVALID_STATE if it senses the file is in an
 *   invalid state
 */
err_t fpgr_create (file_pager *dest, fpgr_params);

/**
 * Allocates a new page and stores result in pgno_dest
 * Returns:
 *   - Forwards errors from i_truncate
 */
err_t fpgr_new (file_pager *p, pgno *pgno_dest);

/**
 * Commits data pointed to by src (assuming page_size) to page pgno
 * Returns:
 *  - Forwards errors from i_write_all
 */
err_t fpgr_commit (file_pager *p, const u8 *src, pgno pgno);

/**
 * Deletes page at pgno.
 * TODO - This doesn't do anything yet because I don't have a
 * good page deletion management
 */
err_t fpgr_delete (file_pager *p, pgno pgno);

/**
 * Fetches page. Expect page to exist
 * Returns:
 *   - INVALID_STATE - page doesn't exist
 *   - ERR_IO - read fails
 */
err_t fpgr_get_expect (file_pager *p, u8 *dest, pgno pgno);
