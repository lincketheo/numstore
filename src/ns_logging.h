#pragma once

typedef enum
{
  DEBUG,
  INFO,
  WARN,
  ERROR,
} log_type;

void ns_log (log_type type, const char *fmt, ...);
