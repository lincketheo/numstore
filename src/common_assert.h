#pragma once

#ifndef NDEBUG
#include "common_logging.h" // fprintf_err
#include <stdlib.h>         // exit
#define ASSERT(expr)                                                          \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          fprintf_err ("BUG! ASSERTION: %s failed. "                          \
                       "Location: (%s):%s:%d\n",                              \
                       #expr, __FILE__, __func__, __LINE__);                  \
          exit (-1);                                                          \
        }                                                                     \
    }                                                                         \
  while (0)

#define TODO(msg)                                                             \
  do                                                                          \
    {                                                                         \
      fprintf_err ("TODO! " msg " "                                           \
                   "Location: (%s):%s:%d\n",                                  \
                   __FILE__, __func__, __LINE__);                             \
      exit (-1);                                                              \
    }                                                                         \
  while (0)
#else
#define ASSERT(expr)
#endif

#define unreachable() ASSERT (0 && "BUG! UNREACHABLE\n")
