#pragma once

typedef enum
{
  SUCCESS = 0,
  ERR_IO = -1,                // Generic System level IO Error,
  ERR_INVALID_STATE = -2,     // Database is in the wrong state,
  ERR_ALREADY_EXISTS = -5,    // A thing already exists
  ERR_DOESNT_EXIST = -6,      // Something doesn't exist
  ERR_NOMEM = -9,             // No memory available
  ERR_PGSTACK_OVERFLOW = -10, // Page stack overflow
  ERR_ARITH = -11,            // Integer overflow or div by 0
  ERR_NOT_LOADED = -12,       // Variable is not loaded into a cursor

  ERR_FALLBACK = -100000,
} err_t;
