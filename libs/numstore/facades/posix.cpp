//
// Created by theo on 12/15/24.
//

#include "numstore/facades/posix.hpp"
#include "numstore/datastruct.hpp"
#include "numstore/errors.hpp"
#include "numstore/logging.hpp"
#include "numstore/facades/stdlib.hpp"

extern "C" {
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>

ns_ret_t
open_f(int *dest, const char *path, const int flags, ...) {
    ASSERT(dest);
    ASSERT(path);

    va_list args;
    va_start(args, flags);

    errno = 0;
    int ret;

    if (flags & O_CREAT) {
        mode_t mode = va_arg(args, mode_t);
        ret = open(path, flags, mode);
    } else {
        ret = open(path, flags);
    }

    va_end(args);

    if (ret < 0) {
        if (errno) {
            ns_log(ERROR, "Failed to call open: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call open for unknown reasons\n");
        return NS_ERROR;
    }

    *dest = ret;
    return NS_OK;
}

ns_ret_t
fdopen_f(FILE **dest, int fd, const char *modes) {
    ASSERT(dest);
    ASSERT(fd > 0);
    ASSERT(modes);

    errno = 0;
    FILE *fp = fdopen(fd, modes);

    if (fp == NULL) {
        if (errno) {
            ns_log(ERROR, "Failed to call fdopen: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call fdopen for unknown reasons\n");
        return NS_ERROR;
    }
    *dest = fp;
    return NS_OK;
}

ns_ret_t
fileno_f(int *dest, FILE *fp) {
    ASSERT(dest);
    ASSERT(fp);

    errno = 0;
    int ret = fileno(fp);
    if (ret <= 0) {
        if (errno) {
            ns_log(ERROR, "Failed to call fileno: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call fileno for unknown reasons\n");
        return NS_ERROR;
    }
    *dest = ret;
    return NS_OK;
}

ns_ret_t
mmap_f(void **dest, void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    ASSERT(dest);
    ASSERT(len > 0);
    ASSERT(fd > 0);

    errno = 0;
    void *ret = mmap(addr, len, prot, flags, fd, offset);
    if (ret == NULL) {
        if (errno) {
            ns_log(ERROR, "Failed to call mmap: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call mmap for unknown reasons\n");
        return NS_ERROR;
    }
    *dest = ret;
    return NS_OK;
}

ns_ret_t
close_f(int fd) {
    ASSERT(fd > 0);

    errno = 0;
    if (close(fd)) {
        if (errno) {
            ns_log(ERROR, "Failed to call close: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call close for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
munmap_f(void *data, size_t len) {
    ASSERT(data);
    ASSERT(len > 0);

    errno = 0;
    if (munmap(data, len)) {
        if (errno) {
            ns_log(ERROR, "Failed to call munmap: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call munmap for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
ftruncate_f(int fd, off_t length) {
    ASSERT(fd > 0);
    ASSERT(length >= 0);

    errno = 0;
    if (ftruncate(fd, length)) {
        if (errno) {
            ns_log(ERROR, "Failed to call ftruncate: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call ftruncate for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
mkdir_f(const char *fname, mode_t mode) {
    ASSERT(fname);

    errno = 0;
    if (mkdir(fname, mode)) {
        if (errno) {
            ns_log(ERROR, "Failed to call mkdir: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call mkdir for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
rmdir_f(const char *fname) {
    ASSERT(fname);

    errno = 0;
    if (rmdir(fname)) {
        if (errno) {
            ns_log(ERROR, "Failed to call rmdir: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call rmdir for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
read_f(size_t *read_out, int fd, void *buf, size_t bytes) {
    ASSERT(read_out);
    ASSERT(fd > 0);
    ASSERT(buf);
    ASSERT(bytes > 0);

    errno = 0;
    ssize_t ret = read(fd, buf, bytes);
    if (ret < 0) {
        if (errno) {
            ns_log(ERROR, "Failed to call read: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call read for unknown reasons\n");
        return NS_ERROR;
    }

    *read_out = (size_t) ret;
    return NS_OK;
}

PRIVATE FILE *
fifoopen(const char *path, const char *mode) {
    if (mkfifo(path, 0666))
        return NULL;

    int flags;
    if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0) {
        flags = O_WRONLY;
    } else if (strcmp(mode, "r+") == 0 || strcmp(mode, "rb+") == 0
               || strcmp(mode, "w+") == 0 || strcmp(mode, "wb+") == 0) {
        flags = O_RDWR;
    } else {
        unreachable();
    }

    int fd = open(path, flags);

    if (fd == -1)
        return NULL;

    return fdopen(fd, mode);
}

ns_ret_t
fifoopen_f(FILE **dest, const char *path, const char *modes) {
    ASSERT(dest);
    ASSERT(path);
    ASSERT(modes);

    errno = 0;
    FILE *ret = fifoopen(path, modes);
    if (ret == NULL) {
        if (errno) {
            ns_log(ERROR, "Failed to call fifoopen: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call fifoopen for unknown reasons\n");
        return NS_ERROR;
    }
    *dest = ret;

    return NS_OK;
}

ns_ret_t stat_f(const char *fname, struct stat *sb) {
    ASSERT(sb);
    ASSERT(fname);

    errno = 0;
    if (stat(fname, sb) == -1) {
        if (errno) {
            ns_log(ERROR, "Failed to call stat: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call stat for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
fstat_f(int fd, struct stat *buf) {
    ASSERT(buf);
    errno = 0;
    if (fstat(fd, buf) == -1) {
        if (errno) {
            ns_log(ERROR, "Failed to call fstat: %s\n", strerror(errno));
            return errno_to_tjl_error(errno);
        }
        ns_log(ERROR, "Failed to call fstat for unknown reasons\n");
        return NS_ERROR;
    }
    return NS_OK;
}

ns_ret_t
mmap_FILE(bytes *dest, FILE *fp, int prot, int flags) {
    ASSERT(dest);
    ASSERT(fp);

    ns_ret_t ret;
    int fd;
    if ((ret = fileno_f(&fd, fp)) != NS_OK) {
        return ret;
    }

    struct stat st{};
    if ((ret = fstat_f(fd, &st)) != NS_OK) {
        return ret;
    }

    size_t size = st.st_size;
    dest->len = size;
    void *data;

    if ((ret = mmap_f(&data, NULL, size, prot, flags, fd, 0)) != NS_OK) {
        return ret;
    }

    dest->data = (byte *) data;
    dest->len = size;

    return NS_OK;
}
}
