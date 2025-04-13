#pragma once

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
