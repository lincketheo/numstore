//
// Created by theo on 12/15/24.
//

#include "numstore/errors.hpp"

extern "C" {
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
fatal_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    ns_log(FATAL, "Aborting program early due to fatal error\n");
    exit(-1);
}
}
