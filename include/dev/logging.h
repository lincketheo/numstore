#pragma once

#include "os/io.h"

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define __LOG_FORMAT_STDOUT(color, level, fmt, ...) \
  stdout_out_io(color "[" level "] %s:%d:%s(): " fmt RESET "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define __LOG_FORMAT_STDERR(color, level, fmt, ...) \
  stderr_out_io(color "[" level "] %s:%d:%s(): " fmt RESET "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define LOG_TRACE(fmt, ...)                                         \
  do {                                                              \
    if (LOG_LEVEL <= LOG_LEVEL_TRACE) {                             \
      __LOG_FORMAT_STDOUT(BOLD_WHITE, "TRACE", fmt, ##__VA_ARGS__); \
    }                                                               \
  } while (0)

#define LOG_DEBUG(fmt, ...)                                        \
  do {                                                             \
    if (LOG_LEVEL <= LOG_LEVEL_DEBUG) {                            \
      __LOG_FORMAT_STDOUT(BOLD_CYAN, "DEBUG", fmt, ##__VA_ARGS__); \
    }                                                              \
  } while (0)

#define LOG_INFO(fmt, ...)                                         \
  do {                                                             \
    if (LOG_LEVEL <= LOG_LEVEL_INFO) {                             \
      __LOG_FORMAT_STDOUT(BOLD_GREEN, "INFO", fmt, ##__VA_ARGS__); \
    }                                                              \
  } while (0)

#define LOG_WARN(fmt, ...)                                          \
  do {                                                              \
    if (LOG_LEVEL <= LOG_LEVEL_WARN) {                              \
      __LOG_FORMAT_STDOUT(BOLD_YELLOW, "WARN", fmt, ##__VA_ARGS__); \
    }                                                               \
  } while (0)

#define LOG_ERROR(fmt, ...)                                       \
  do {                                                            \
    if (LOG_LEVEL <= LOG_LEVEL_ERROR) {                           \
      __LOG_FORMAT_STDOUT(BOLD_RED, "ERROR", fmt, ##__VA_ARGS__); \
    }                                                             \
  } while (0)

#define LOG_SUCCESS(fmt, ...) \
  __LOG_FORMAT_STDOUT(BOLD_GREEN, "SUCCESS", fmt, ##__VA_ARGS__)

#define LOG_FAILURE(fmt, ...) \
  __LOG_FORMAT_STDOUT(BOLD_RED, "FAILURE", fmt, ##__VA_ARGS__)
