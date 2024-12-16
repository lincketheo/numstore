//
// Created by theo on 12/15/24.
//

#ifndef STDLIB_HPP
#define STDLIB_HPP

#include "numstore/errors.hpp"

extern "C" {
#include <stdio.h>

// Converts errno to a domain level error
ns_ret_t errno_to_tjl_error(int err);

//////////////////////////////////////////
/// Facades STDLIB_FUNC_f
ns_ret_t fopen_f(FILE **dest, const char *fname, const char *mode);

ns_ret_t malloc_f(void **dest, size_t len);

ns_ret_t realloc_f(void **b, size_t newlen);

ns_ret_t fseek_f(FILE *fp, long int offset, int whence);

ns_ret_t ftell_f(size_t *dest, FILE *fp);

ns_ret_t fclose_f(FILE *fp);

ns_ret_t fgets_f(char *s, int n, FILE *fp);

ns_ret_t remove_f(const char *fname);

ns_ret_t fwrite_f(const void *ptr, size_t size, size_t n, FILE *s);

ns_ret_t fread_f(void *ptr, size_t size, size_t *n, FILE *s);

ns_ret_t fread_expect_all(void *ptr, size_t size, size_t n, FILE *s);

ns_ret_t free_f(void *ptr);
}

#endif // STDLIB_HPP
