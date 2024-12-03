#pragma once

#include "numstore_memory.h"

#include <stdbool.h>
#include <stdlib.h>

bool file_exists_any_type (const char *_fname);

bool dir_exists (const char *dname);

bool file_exists (const char *dname);

typedef enum
{
  FER_EXISTS,
  FER_DOESNT_EXIST,
  FER_MEMORY_ERROR,
} fexists_ret;

fexists_ret file_exists_in_dir (linmem *m, const char *dname,
                                const char *_fname);

int create_dir (const char *dirname, int mode);

typedef struct
{
  enum
  {
    FCR_SUCCESS,
    FCR_OS_FAILURE,
    FCR_ALREADY_EXISTS,
    FCR_ARGS_TOO_LONG,
  } type;
  int errno_if_os_failure;
} fcreate_ret;

fcreate_ret create_file_in_dir (const char *dname, const char *fname);
