//
// Created by theo on 12/15/24.
//

#ifndef POSIX_HPP
#define POSIX_HPP

#include "numstore/datastruct.hpp"
#include "numstore/errors.hpp"

extern "C" {
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

//////////////////////////////////////////
/// Facades POSIX_FUNC_f
ns_ret_t mkfifo_f(const char *path, __mode_t mode);

ns_ret_t open_f(int *dest, const char *path, int flags, ...);

ns_ret_t fdopen_f(FILE **dest, int fd, const char *modes);

ns_ret_t fileno_f(int *dest, FILE *fp);

ns_ret_t mmap_f(void **dest, void *addr, size_t len, int prot, int flags,
                int fd, off_t offset);

ns_ret_t close_f(int fd);

ns_ret_t munmap_f(void *data, size_t len);

ns_ret_t ftruncate_f(int fd, off_t length);

ns_ret_t mkdir_f(const char *fname, mode_t mode);

ns_ret_t rmdir_f(const char *fname);

ns_ret_t read_f(size_t *read, int fd, void *buf, size_t bytes);

ns_ret_t fifoopen_f(FILE **dest, const char *path, const char *mode);

ns_ret_t stat_f(const char *fname, struct stat *sb);

ns_ret_t fstat_f(int fd, struct stat *buf);

ns_ret_t mmap_FILE(bytes *dest, FILE *fp, int prot, int flags);
}

#endif // POSIX_HPP
