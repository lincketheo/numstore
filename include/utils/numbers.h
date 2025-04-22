#pragma once

#include "dev/errors.h"
#include "sds.h"
#include "types.h"

err_t smlst_dbl_fctr (u32 *cap, u32 len, u32 nbytes);
err_t parse_i32_expect (i32 *dest, const string data);
err_t parse_f32_expect (f32 *dest, const string data);
