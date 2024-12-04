#pragma once

#include "ns_logging.h"

#include <stdlib.h> // abort

typedef enum
{
  NS_OK = 0,   // Successful result
  NS_PERM,     // Access permission denied
  NS_NOMEM,    // Out of memory
  NS_IOERR,    // I/O error
  NS_CORRUPT,  // Corruption detected
  NS_NOTFOUND, // Not found
  NS_BUSY,     // Resource busy
  NS_CANTOPEN, // Unable to open resource
  NS_EINVAL,   // Invalid argument
#ifndef NDEBUG
  NS_ERROR, // Fallback for non debug builds
#endif
} ns_ret_t;

ns_ret_t errno_to_ns_error (int err);

#define fail() abort ()

#define unreachable()                                                         \
  do                                                                          \
    {                                                                         \
      ns_log (ERROR,                                                          \
              "BUG! Unreachable! "                                            \
              "(%s):%s:%d\n",                                                 \
              __FILE__, __func__, __LINE__);                                  \
      fail ();                                                                \
    }                                                                         \
  while (0)

#ifndef NDEBUG

#define ASSERT(expr)                                                          \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          ns_log (ERROR,                                                      \
                  "BUG! ASSERTION: " #expr " failed. "                        \
                  "Location: (%s):%s:%d\n",                                   \
                  __FILE__, __func__, __LINE__);                              \
          fail ();                                                            \
        }                                                                     \
    }                                                                         \
  while (0)

#define TODO(msg)                                                             \
  do                                                                          \
    {                                                                         \
      ns_log (ERROR,                                                          \
              "TODO! " msg " "                                                \
              "Location: (%s):%s:%d\n",                                       \
              __FILE__, __func__, __LINE__);                                  \
      fail ();                                                                \
    }                                                                         \
  while (0)

#else

#define ASSERT(expr)

#endif
