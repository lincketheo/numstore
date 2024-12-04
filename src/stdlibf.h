#pragma once

#include "ns_errors.h"

#include <stdio.h>

typedef struct
{
  FILE *fp;
  ns_ret_t stat;
} ns_FILE_ret;

ns_ret_t ns_fclose (FILE *fp);

ns_FILE_ret ns_fopen (const char *fname, const char *mode);

