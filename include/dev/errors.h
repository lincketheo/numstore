#pragma once

typedef enum
{
  SUCCESS = 0,
  ERR_IO = -1,             // Generic System level IO Error,
  ERR_INVALID_STATE = -2,  // Database is in the wrong state,
  ERR_INVALID_INPUT = -3,  // Input is too long / malformed - thing 400,
  ERR_OVERFLOW = -4,       // User Entered value overflows limits,
  ERR_ALREADY_EXISTS = -5, // A thing already exists
  ERR_DOESNT_EXIST = -6,   // Something doesn't exist
  ERR_OUT_OF_BOUNDS = -7,  // For out of bounds indexing methods
} err_t;
