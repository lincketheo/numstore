#pragma once

typedef enum
{
  SUCCESS = 0,
  ERR_IO = -1,            // Generic System level IO Error,
  ERR_INVALID_STATE = -2, // Database is in the wrong state,
  ERR_INVALID_INPUT = -3, // Input is too long / malformed - thing 400,
  ERR_OVERFLOW = -4,      // User Entered value overflows limits,
} err_t;
