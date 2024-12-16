//
// Created by theo on 12/15/24.
//

#ifndef NS_ERRORS_HPP
#define NS_ERRORS_HPP

#include "macros.hpp"
#include "numstore/logging.hpp"

extern "C" {
#include <stdlib.h>

typedef enum {
    NS_OK = 0, // Successful result
    NS_PERM, // Access permission denied
    NS_NOMEM, // Out of memory
    NS_IOERR, // I/O error
    NS_CORRUPT, // Corruption detected
    NS_NOTFOUND, // Not found
    NS_UNSUPPORTED, // Operation not supported
    NS_BUSY, // Resource busy
    NS_CANTOPEN, // Unable to open resource
    NS_EINVAL, // Invalid argument
    NS_PIPE, // Invalid seek
    NS_NOTDIR, // Not a directory
    NS_NOENT,
    NS_BADF,
    NS_ERROR, // Fallback - use sparingly
} ns_ret_t;

#define ns_ret_t_fallthrough(expr) \
  do                                \
    {                               \
      ns_ret_t _ret;               \
      if ((_ret = expr) != NS_OK)  \
        {                           \
          return _ret;              \
        }                           \
    }                               \
  while (0)

#define ns_ret_t_onfail(expr, label) \
      if ((ret = expr) != NS_OK)  \
        {                           \
          goto label;           \
        }

static inline void
tjl_ret_unwrap(ns_ret_t ret) {
    if (ret != NS_OK) {
        ns_log(ERROR, "Operation failed\n");
        exit(-1);
    }
}

/**
 * @brief Print an error message then exit
 */
NORETURN void fatal_error(const char *format, ...);

#define fatal_error_if(expr, ...)                                         \
  do                                                                      \
    {                                                                     \
      if (__builtin_expect (!!(expr), 0))                                 \
        {                                                                 \
          if (errno)                                                      \
            perror (#expr);                                               \
          fatal_error ("Expression: %s failed. Aborting early\n", #expr); \
        }                                                                 \
    }                                                                     \
  while (0)

#define unreachable() \
  fatal_error ("BUG! Unreachable! (%s):%s:%d\n", __FILE__, __func__, __LINE__);

#ifndef NDEBUG

#define ASSERT(expr)                                        \
  do                                                        \
    {                                                       \
      if (!(expr))                                          \
        {                                                   \
          fatal_error ("BUG! ASSERTION: " #expr " failed. " \
                       "Location: (%s):%s:%d\n",            \
                       __FILE__, __func__, __LINE__);       \
        }                                                   \
    }                                                       \
  while (0)

#define TODO(msg)                                 \
  do                                              \
    {                                             \
      fatal_error ("TODO! " msg " "               \
                   "Location: (%s):%s:%d\n",      \
                   __FILE__, __func__, __LINE__); \
    }                                             \
  while (0)

#else

#define ASSERT(expr)

// TODO left undefined for compile time error
#endif
}

#endif //NS_ERRORS_HPP
