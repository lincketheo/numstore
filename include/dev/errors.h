#pragma once

typedef enum
{
  SUCCESS = 0,
  ERR_IO = -1,             // Generic System level IO Error,
  ERR_INVALID_STATE = -2,  // Database is in the wrong state,
  ERR_INVALID_INPUT = -3,  // Input is too long / malformed - think 400,
  ERR_OVERFLOW = -4,       // User Entered value overflows limits,
  ERR_ALREADY_EXISTS = -5, // A thing already exists
  ERR_DOESNT_EXIST = -6,   // Something doesn't exist
  ERR_ARITH = -7,          // Arithmetic error like div 0 or int overflow
  ERR_NOMEM = -9,          // No memory available
} err_t;

#define err_t_wrap(expr)  \
  do                      \
    {                     \
      err_t __ret = expr; \
      if (__ret)          \
        {                 \
          return __ret;   \
        }                 \
    }                     \
  while (0)

#define err_t_cwrap(condexpr, ret) \
  do                               \
    {                              \
      if (condexpr)                \
        {                          \
          return ret;              \
        }                          \
    }                              \
  while (0)
