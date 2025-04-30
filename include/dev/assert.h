#pragma once

#include "intf/logging.h"

////////////////////////////// Debug
#if !defined(NDEBUG) && !defined(__clang_analyzer__)

#define ASSERT(expr)                                          \
  do                                                          \
    {                                                         \
      if (!(expr))                                            \
        {                                                     \
          i_log_assert ("%s failed at %s:%d (%s)\n",          \
                        #expr, __FILE__, __LINE__, __func__); \
          *(volatile int *)0 = 1;                             \
        }                                                     \
    }                                                         \
  while (0)

#define ASCOPE(expr) expr

#define DEFINE_DBG_ASSERT_I(type, name, variable) \
  __attribute__ ((unused)) static inline void name##_assert (const type *variable)

////////////////////////////// Release
#else

#define ASSERT(expr)

#define ASCOPE(expr)

#define DEFINE_DBG_ASSERT_I(type, name, variable)                                                           \
  __attribute__ ((unused)) static inline void name##_assert (const type *variable __attribute__ ((unused))) \
  {                                                                                                         \
    /* do nothing*/                                                                                         \
  }                                                                                                         \
  __attribute__ ((unused)) static inline void name##_assert__ (const type *variable __attribute__ ((unused)))

#endif
