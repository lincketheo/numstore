#pragma once

#include "core/intf/types.h"

#include <complex.h>
#include <math.h>

#define i_creal_64(f) (creal (f))
#define i_cimag_64(f) (cimag (f))

#define i_cabs_sqrd_64(f) ((creal (f) * creal (f)) + ((cimag (f) * cimag (f))))
#define i_cabs_64(f) cabsf (f)
#define i_fabs_32(f) fabsf (f)
