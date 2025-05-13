#pragma once

#include "ds/strings.h"   // string
#include "errors/error.h" // err_t
#include "intf/types.h"   // u32

/**
 * Returns:
 *   - ERR_ARITH - Arithmetic error (e.g. bounds or finite etc)
 */
err_t parse_i32_expect (i32 *dest, const string data, error *e);
err_t parse_f32_expect (f32 *dest, const string data, error *e);
