#pragma once

#include "core/ds/strings.h"   // string
#include "core/errors/error.h" // err_t
#include "core/intf/types.h"   // u32

/**
 * Returns:
 *   - ERR_ARITH - Arithmetic error (e.g. bounds or finite etc)
 */
err_t parse_i32_expect (i32 *dest, const string data, error *e);
err_t parse_f32_expect (f32 *dest, const string data, error *e);
f32 py_mod_f32 (f32 num, f32 denom);
i32 py_mod_i32 (i32 num, i32 denom);
