#pragma once

#include "ns_data_structures.h" // ns_ret_t
#include "ns_errors.h"          // srange

#include <stdio.h>

// Create Database
typedef struct
{
  const char *dbname;
} ns_create_db_args;

ns_ret_t ns_create_db (ns_create_db_args args);

// Create Dataset
typedef struct
{
  const char *dbname;
  const char *dsname;
} ns_create_ds_args;

ns_ret_t ns_create_ds (ns_create_ds_args args);

// Read srange from input data set to output file pointer
typedef struct
{
  const char *dbname;
  const char *dsname;
  FILE *fp;
  srange s;
} ns_read_srange_ds_fp_args;

ns_ret_t ns_read_srange_ds_fp (ns_read_srange_ds_fp_args args);
