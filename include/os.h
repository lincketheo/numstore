#pragma once

#include "common/types.h"

/////////////////////// Allocation
void* rads_malloc(size_t bytes);

void rads_free(void* ptr);

void* rads_realloc(void* ptr, int bytes);

/////////////////////// Files
typedef struct rads_file rads_file;

rads_file* rads_open(const char* fname, int read, int write);

int rads_close(rads_file* fp);

i64 rads_read(rads_file* fp, void* dest, int n, i64 offset);

i64 rads_write(rads_file* fp, const void* src, int n, i64 offset);

/////////////////////// Logging
void log_trace(const char* fmt, ...);

void log_debug(const char* fmt, ...);

void log_info(const char* fmt, ...);

void log_warn(const char* fmt, ...);

void log_error(const char* fmt, ...);
