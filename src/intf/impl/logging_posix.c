#include "intf/logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void
i_log_internal (
    const char *fullfile,
    int line,
    const char *func,
    const char *prefix,
    const char *color,
    const char *fmt,
    ...)
{
#ifndef __clang_analyzer__
  // Extract basename from __FILE__
  const char *file = strrchr (fullfile, '/');
#ifdef _WIN32
  const char *bslash = strrchr (fullfile, '\\');
  if (!file || (bslash && bslash > file))
    file = bslash;
#endif
  file = file ? file + 1 : fullfile;

  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "%s[%-8.8s] %s:%d:%s(): ", color, prefix, file, line, func);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "%s", RESET);
  va_end (args);
#else
  (void)fullfile;
  (void)line;
  (void)func;
  (void)prefix;
  (void)color;
  (void)fmt;
#endif
}
