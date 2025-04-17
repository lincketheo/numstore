#pragma once

#define SUCCESS 0
#define ERR_IO -1            // Generic System level IO Error
#define ERR_INVALID_STATE -2 // Database is in the wrong state
#define ERR_INVALID_INPUT -3 // Input is too long / malformed - thing 400

#define err_unwrap(expr)                                               \
  do                                                                   \
    {                                                                  \
      int __res = expr;                                                \
      if (__res != SUCCESS)                                            \
        {                                                              \
          i_log_warn ("Error encountered in expression: %s\n", #expr); \
          return __res;                                                \
        }                                                              \
    }                                                                  \
  while (0)
