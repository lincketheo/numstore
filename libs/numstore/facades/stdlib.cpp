//
// Created by theo on 12/15/24.
//

#include "numstore/facades/stdlib.hpp"

#include "numstore/errors.hpp"

extern "C" {
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

ns_ret_t errno_to_tjl_error(const int err) {
  ASSERT(err != 0);

  switch (err) {
  case EPERM:
  case EACCES:
    return NS_PERM;
  case ENOMEM:
    return NS_NOMEM;
  case EIO:
    return NS_IOERR;
  case EBUSY:
    return NS_BUSY;
  case EINVAL:
  case ENOTDIR:
    return NS_EINVAL;
  case ENFILE:
  case EMFILE:
    return NS_CANTOPEN;
  case ESPIPE:
    return NS_PIPE;
  default:
    unreachable();
  }
}

ns_ret_t fopen_f(FILE **dest, const char *fname, const char *mode) {
  ASSERT(dest);
  ASSERT(fname);
  ASSERT(mode);

  errno = 0;
  FILE *ret = fopen(fname, mode);
  if (ret == NULL) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  *dest = ret;
  return NS_OK;
}

ns_ret_t malloc_f(void **dest, const size_t len) {
  ASSERT(dest);
  ASSERT(len > 0);

  errno = 0;
  void *ret = malloc(len);
  if (ret == NULL) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  *dest = ret;
  return NS_OK;
}

ns_ret_t realloc_f(void **b, const size_t newlen) {
  ASSERT(b);
  ASSERT(*b);
  ASSERT(newlen > 0);

  errno = 0;
  void *ret = realloc(*b, newlen);
  if (ret == NULL) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  *b = ret;
  return NS_OK;
}

ns_ret_t fseek_f(FILE *fp, const long int offset, const int whence) {
  ASSERT(fp);

  errno = 0;
  if (fseek(fp, offset, whence)) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  return NS_OK;
}

ns_ret_t ftell_f(size_t *dest, FILE *fp) {
  ASSERT(dest);
  ASSERT(fp);

  errno = 0;
  ssize_t ret = ftell(fp);
  if (ret < 0) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  *dest = ret;
  return NS_OK;
}

ns_ret_t fclose_f(FILE *fp) {
  ASSERT(fp);

  errno = 0;
  if (fclose(fp)) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  return NS_OK;
}

ns_ret_t fgets_f(char *s, const int n, FILE *fp) {
  ASSERT(s);
  ASSERT(n > 0);
  ASSERT(fp);

  errno = 0;
  if (fgets(s, n, fp)) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  return NS_OK;
}

ns_ret_t remove_f(const char *fname) {
  ASSERT(fname);

  errno = 0;
  if (remove(fname)) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }
  return NS_OK;
}

ns_ret_t fwrite_f(const void *ptr, const size_t size, const size_t n, FILE *s) {
  ASSERT(ptr);
  ASSERT(size > 0);
  ASSERT(n > 0);
  ASSERT(s);

  errno = 0;
  size_t written = fwrite(ptr, size, n, s);
  if (written != n) {
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    return NS_ERROR;
  }

  return NS_OK;
}

ns_ret_t fread_f(void *ptr, const size_t size, size_t *n, FILE *s) {
  ASSERT(ptr);
  ASSERT(size > 0);
  ASSERT(n);
  ASSERT(s);

  errno = 0;
  size_t read = fread(ptr, size, *n, s);

  if (read != *n) {
    *n = read;
    if (errno) {
      return errno_to_tjl_error(errno);
    }
    if (ferror(s)) {
      return NS_IOERR;
    }
  }

  return NS_OK;
}

ns_ret_t free_f(void *ptr) {
  ASSERT(ptr);
  free(ptr);
  return NS_OK;
}
}
