#pragma once
/**
 * Assert philosophy
 *
 * This is a little different than normal. An
 * ASSERT just crashes the program
 *
 * A DBG_ASSERT
 */

#ifndef NASSERTS

#include <stdio.h>

#define ASSERT(expr)                                     \
  do                                                     \
    {                                                    \
      if (!(expr))                                       \
        {                                                \
          fprintf (stderr, "%s failed at %s:%d (%s)\n",  \
                   #expr, __FILE__, __LINE__, __func__); \
          *(volatile int *)0 = 1;                        \
        }                                                \
    }                                                    \
  while (0)

#define DEFINE_DBG_ASSERT(type, name, variable) \
  __attribute__ ((unused)) void name##_assert (const type *variable)

#define DEFINE_PROD_ASSERT(type, name, variable) \
  __attribute__ ((unused)) void name##_assert (const type *variable)

#else

// TODO - Think of a better way to compile this function out
#define DEFINE_DBG_ASSERT(type, name, variable)                                    \
  __attribute__ ((unused)) static inline void name##_assert (const type *variable) \
  {                                                                                \
    /* do nothing*/                                                                \
  }

#endif
