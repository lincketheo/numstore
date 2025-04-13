#pragma once

#include "common/types.h"
#include "dev/assert.h"
#include "os/types.h"

/////////////////////// Allocation
void *i_malloc (u64 bytes);

void *i_calloc (u64 n, u64 size);

void i_free (void *ptr);

void *i_realloc (void *ptr, int bytes);

/////////////////////// Files
typedef struct i_file i_file;

DEFINE_DBG_ASSERT (i_file, i_file, p);

i_file *i_open (const string *fname, int read, int write);

int i_close (i_file *fp);

i64 i_read_some (i_file *fp, void *dest, u64 n, u64 offset);

i64 i_read_all (i_file *fp, void *dest, u64 n, u64 offset);

i64 i_write_some (i_file *fp, const void *src, u64 n, u64 offset);

int i_write_all (i_file *fp, const void *src, u64 n, u64 offset);

int i_truncate (i_file *fp, u64 bytes);

i64 i_file_size (i_file *fp);

/////////////////////// Logging
#ifndef NLOGGING
void i_log_trace (const char *fmt, ...);

void i_log_debug (const char *fmt, ...);

void i_log_info (const char *fmt, ...);

void i_log_warn (const char *fmt, ...);

void i_log_error (const char *fmt, ...);

void i_log_failure (const char *fmt, ...);

void i_log_success (const char *fmt, ...);
#else
#define i_log_trace (fmt, __VA_ARGS__)

#define i_log_debug (fmt, __VA_ARGS__)

#define i_log_info (fmt, __VA_ARGS__)

#define i_log_warn (fmt, __VA_ARGS__)

#define i_log_error (fmt, __VA_ARGS__)

#define i_log_failure (fmt, __VA_ARGS__)

#define i_log_success (fmt, __VA_ARGS__)
#endif
