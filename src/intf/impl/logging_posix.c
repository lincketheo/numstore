#include "intf/logging.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void
i_log_internal (
    const char *prefix,
    const char *color,
    const char *fmt,
    ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "%s[%-8.8s]: ", color, prefix);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "%s", RESET);
  va_end (args);
}
