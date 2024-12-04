#pragma once

#include "ns_dtype.h"
#include <stdio.h>

typedef enum
{
  START,
  READING_DATA,
} ns_ds_state_t;

typedef struct
{
  dtype type;
  ns_ds_state_t state;
  FILE *fp;
} ns_ds_state;

