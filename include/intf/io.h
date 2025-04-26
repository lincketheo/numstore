#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/types.h"

/////////////////////// Files
typedef struct i_file i_file;

struct i_file
{
  int fd;
};

err_t i_open (i_file *dest, const string fname, int read, int write);
err_t i_close (i_file *fp);
i64 i_read_some (i_file *fp, void *dest, u64 n, u64 offset);
i64 i_read_all (i_file *fp, void *dest, u64 n, u64 offset);
i64 i_write_some (i_file *fp, const void *src, u64 n, u64 offset);
err_t i_write_all (i_file *fp, const void *src, u64 n, u64 offset);
err_t i_truncate (i_file *fp, u64 bytes);
i64 i_file_size (i_file *fp);
err_t i_remove_quiet (const string fname);
err_t i_mkstemp (i_file *dest, string tmpl);
err_t i_unlink (const string name);

// Slight wrappers
err_t i_access_rw (const string fname);
bool i_exists_rw (const string fname);
