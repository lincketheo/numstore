#pragma once

#include "numstore_dtype.h"
#include <stdio.h>

typedef struct
{
  dtype type;
} nsdsheader;

int nsdsheader_fwrite (FILE *dest, const nsdsheader src);

int nsdsheader_fread (nsdsheader *dest, FILE *src);
