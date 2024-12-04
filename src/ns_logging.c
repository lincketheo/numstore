#include "ns_logging.h"

#include <stdarg.h>
#include <stdio.h>

void
ns_log (log_type type, const char *fmt, ...)
{
  va_list args;

  switch (type)
    {
    case DEBUG:
      fprintf (stderr, "[DEBUG]: ");
      break;
    case INFO:
      fprintf (stderr, "[INFO]: ");
      break;
    case WARN:
      fprintf (stderr, "[WARN]: ");
      break;
    case ERROR:
      fprintf (stderr, "[ERROR]: ");
      break;
    default:
      fprintf (stderr, "[UNKNOWN]: ");
      break;
    }

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
}
