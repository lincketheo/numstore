//
// Created by theo on 12/15/24.
//

#ifndef FILES_HPP
#define FILES_HPP

#include "numstore/datastruct.hpp"
#include "numstore/errors.hpp"

extern "C" {
#include <stdio.h> // FILE

///////////////////////////////////////////////////////
///////// File utils

ns_ret_t fsize(size_t *dest, FILE *fp, size_t size);

ns_ret_t fread_malloc(bytes *dest, FILE *fp, size_t size, size_t len);

ns_ret_t fread_all_malloc_noseek(bytes *dest, FILE *fp, size_t size, size_t limit);

ns_ret_t fread_all_mmap_seek(bytes *dest, FILE *fp, size_t size, int prot, int flags);

ns_ret_t fread_discard(FILE *file, size_t size, size_t *n);

ns_ret_t is_seekable(bool *dest, FILE *fp);

ns_ret_t fcopy(FILE *ifp, FILE *ofp);

ns_ret_t fread_zero_top(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t fread_shift_half(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t fread_shift_half_zero_top(void *ptr, size_t size, size_t *n, FILE *fp);

ns_ret_t mmap_create_wrb(bytes *dest, const char *fname, size_t bytes);

ns_ret_t mmap_create_wrb_wbase(bytes *dest, const char *base, const char *fname, size_t size, size_t len);

ns_ret_t is_valid_folder(bool *dest, const char *fname);
}

#endif //FILES_HPP
