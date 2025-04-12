#include "os/io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

///////////////////////////////////// Allocation Routines

static void *rads_malloc(size_t size) {
    return malloc(size);
}

static void rads_free(void *ptr) {
    free(ptr);
}

static void *rads_realloc(void *ptr, int newSize) {
    return realloc(ptr, newSize);
}

static int rads_init(void *arg __attribute__((unused))) {
    return 0;
}

static void rads_shutdown(void *ptr __attribute__((unused))) {
}

static rads_alloc alloc = {
    rads_malloc,
    rads_free,
    rads_realloc
};

rads_alloc* get_rads_alloc(void) {
    return &alloc;
}

///////////////////////////////////// File and File Routines

typedef struct {
    rads_file base;
    int fd;
} rads_file_internal;

static int rads_close(rads_file *file) {
    rads_file_internal *internal = (rads_file_internal *)file;
    return close(internal->fd);
}

static int rads_read(rads_file *file, void* dest, int n, u64 offset) {
    rads_file_internal *internal = (rads_file_internal *)file;
    if (lseek(internal->fd, offset, SEEK_SET) == (off_t)-1) return -1;
    return read(internal->fd, dest, n);
}

static int rads_write(rads_file *file, const void* src, int n, u64 offset) {
    rads_file_internal *internal = (rads_file_internal *)file;
    if (lseek(internal->fd, offset, SEEK_SET) == (off_t)-1) return -1;
    return write(internal->fd, src, n);
}

static int rads_truncate(rads_file *file, u64 size) {
    rads_file_internal *internal = (rads_file_internal *)file;
    return ftruncate(internal->fd, size);
}

static int rads_sync(rads_file *file, int flags __attribute__((unused))) {
    rads_file_internal *internal = (rads_file_internal *)file;
    return fsync(internal->fd);
}

static int rads_size(rads_file *file, i64 *pSize) {
    rads_file_internal *internal = (rads_file_internal *)file;
    struct stat st;
    if (fstat(internal->fd, &st) == -1) return -1;
    *pSize = st.st_size;
    return 0;
}

static int rads_lock(rads_file *file __attribute__((unused)), int lockType __attribute__((unused))) {
    return 0;
}

static int rads_unlock(rads_file *file __attribute__((unused)), int lockType __attribute__((unused))) {
    return 0;
}

static rads_io_methods methods = {
    rads_close,
    rads_read,
    rads_write,
    rads_truncate,
    rads_sync,
    rads_size,
    rads_lock,
    rads_unlock
};

rads_io_methods* get_rads_io_methods(void) {
    return &methods;
}

///////////////////////////////////// OS Level functions

static int rads_vfs_open(rads_vfs *vfs __attribute__((unused)), rads_file* dest, const char* fname, int flags) {
    rads_file_internal *internal = (rads_file_internal *)dest;
    int fd = open(fname, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) return -1;
    internal->fd = fd;
    internal->base.methods = get_rads_io_methods();
    return 0;
}

static int rads_vfs_close(rads_vfs *vfs __attribute__((unused))) {
    return 0;
}

static int rads_vfs_delete(rads_vfs *vfs __attribute__((unused)), const char *zName, int syncDir __attribute__((unused))) {
    return unlink(zName);
}

static rads_vfs vfs = {
    rads_vfs_open,
    rads_vfs_close,
    rads_vfs_delete
};

rads_vfs* get_rads_vfs(void) {
    return &vfs;
}

///////////////////////////////////// Logger

static void rads_log_trace(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "TRACE: ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

static void rads_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "DEBUG: ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

static void rads_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "INFO: ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

static void rads_log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "WARN: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static void rads_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static rads_logger logger = {
    rads_log_trace,
    rads_log_debug,
    rads_log_info,
    rads_log_warn,
    rads_log_error
};

rads_logger* get_rads_logger(void) {
    return &logger;
}

