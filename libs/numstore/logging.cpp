//
// Created by theo on 12/15/24.
//

#include "numstore/logging.hpp"

extern "C" {
#include <stdarg.h>
#include <stdio.h>

static void log_prefix(const log_type type) {
  switch (type) {
  case TRACE:
    fprintf(stderr, "[TRACE]: ");
    break;
  case DEBUG:
    fprintf(stderr, "[DEBUG]: ");
    break;
  case INFO:
    fprintf(stderr, "[INFO]: ");
    break;
  case WARN:
    fprintf(stderr, "[WARN]: ");
    break;
  case ERROR:
    fprintf(stderr, "[ERROR]: ");
    break;
  case FATAL:
    fprintf(stderr, "[FATAL]: ");
    break;
  default:
    fprintf(stderr, "[UNKNOWN]: ");
    break;
  }
}

void ns_log(const log_type type, const char *fmt, ...) {
  log_prefix(type);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void ns_log_inl(const log_type type, const char *fmt, ...) {
  fprintf(stderr, "\r");
  log_prefix(type);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void ns_log_inl_done() { fprintf(stderr, "\n"); }
}
