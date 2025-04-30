#pragma once

/////////////////////// Logging
void i_perror (const char *msg);

#ifndef NLOGGING

void i_log_trace (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_debug (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_info (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_warn (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_error (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_assert (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_failure (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void i_log_passed (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#else

#define i_log_trace (fmt, __VA_ARGS__)
#define i_log_debug (fmt, __VA_ARGS__)
#define i_log_info (fmt, __VA_ARGS__)
#define i_log_warn (fmt, __VA_ARGS__)
#define i_log_error (fmt, __VA_ARGS__)
#define i_log_assert (fmt, __VA_ARGS__)
#define i_log_failure (fmt, __VA_ARGS__)
#define i_log_passed (fmt, __VA_ARGS__)

#endif
