#pragma once

#include "intf/logging.h"

#define crash()                 \
  do                            \
    {                           \
      *(volatile int *)0 = 1;   \
      __builtin_unreachable (); \
    }                           \
  while (0)

#define UNREACHABLE() \
  do                  \
    {                 \
      ASSERT (0);     \
      crash ();       \
    }                 \
  while (0)

////////////////////////////// Debug
#if !defined(NDEBUG) && !defined(__clang_analyzer__)

#define panic()                 \
  do                            \
    {                           \
      i_log_error ("PANIC!\n"); \
      crash ();                 \
    }                           \
  while (0)

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

#define panic() typedef char PANIC_in_release_mode_is_not_allowed[-1]

#define ASSERT(expr)

#define ASCOPE(expr)

#define DEFINE_DBG_ASSERT_I(type, name, variable)                                                           \
  __attribute__ ((unused)) static inline void name##_assert (const type *variable __attribute__ ((unused))) \
  {                                                                                                         \
    /* do nothing*/                                                                                         \
  }                                                                                                         \
  __attribute__ ((unused)) static inline void name##_assert__ (const type *variable __attribute__ ((unused)))

#endif

#define ASSERT_PTR_IS_IDX(base, ptr, idx)     \
  do                                          \
    {                                         \
      ASSERT (base);                          \
      ASSERT (ptr);                           \
      ASSERT ((u8 *)ptr >= (u8 *)base);       \
      ASSERT ((u8 *)ptr - (u8 *)base == idx); \
    }                                         \
  while (0)
