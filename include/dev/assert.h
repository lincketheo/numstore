#pragma once

#include "os/logging.h"

#ifndef NDEBUG

#define ASSERT(expr)                                         \
  do                                                         \
    {                                                        \
      if (!(expr))                                           \
        {                                                    \
          i_log_error ("%s failed at %s:%d (%s)\n",          \
                       #expr, __FILE__, __LINE__, __func__); \
          *(volatile int *)0 = 1;                            \
        }                                                    \
    }                                                        \
  while (0)

#define DEFINE_DBG_ASSERT_H(type, name, variable) \
  __attribute__ ((unused)) void name##_assert (const type *variable)

#define DEFINE_DBG_ASSERT_I(type, name, variable) \
  __attribute__ ((unused)) void name##_assert (const type *variable)

#else

#define ASSERT(expr)

// TODO - Think of a better way to compile this function out
#define DEFINE_DBG_ASSERT_H(type, name, variable) \
  __attribute__ ((unused)) void name##_assert (const type *variable __attribute__ ((unused)))

// TODO - Think of a better way to compile this function out
#define DEFINE_DBG_ASSERT_I(type, name, variable)                                             \
  __attribute__ ((unused)) void name##_assert (const type *variable __attribute__ ((unused))) \
  {                                                                                           \
    /* do nothing*/                                                                           \
  }                                                                                           \
  __attribute__ ((unused)) static inline void name##_assert__ (const type *variable __attribute__ ((unused)))

#endif
