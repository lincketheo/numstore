#include "intf/logging.h"
// DO NOT INCLUDE assert

#include <stdarg.h>
#include <stdio.h>

// Regular colors
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"

// Bold colors
#define BOLD_BLACK "\033[1;30m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;37m"

// Reset
#define RESET "\033[0m"

/////////////////////// Logging
#ifndef NLOGGING
static void
i_log_internal (
    const char *prefix,
    const char *color,
    const char *fmt,
    va_list args)
    __attribute__ ((format (printf, 3, 0)));

static void
i_log_internal (
    const char *prefix,
    const char *color,
    const char *fmt,
    va_list args)
{
  // Clangd is historically buggy in this error
#ifndef __clang_analyzer__
  fprintf (stderr, "%s[%-8.8s]:   ", color, prefix);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "%s", RESET);
#else
  (void)prefix;
  (void)color;
  (void)fmt;
  (void)args;
#endif
}

//// Log level wrappers
void
i_log_trace (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("TRACE", BOLD_BLACK, fmt, args);
  va_end (args);
}

void
i_log_debug (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("DEBUG", BLUE, fmt, args);
  va_end (args);
}

void
i_log_info (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("INFO", GREEN, fmt, args);
  va_end (args);
}

void
i_log_warn (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("WARN", YELLOW, fmt, args);
  va_end (args);
}

void
i_log_error (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("ERROR", RED, fmt, args);
  va_end (args);
}

void
i_log_assert (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("ASSERT", RED, fmt, args);
  va_end (args);
}

void
i_log_failure (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("FAILURE", BOLD_RED, fmt, args);
  va_end (args);
}

void
i_log_passed (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("PASSED", BOLD_GREEN, fmt, args);
  va_end (args);
}
#endif
