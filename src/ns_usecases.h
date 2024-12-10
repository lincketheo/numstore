#pragma once

#include "ns_dtype.h"
#include "ns_errors.h" // srange

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
  dtype type;
} ns_create_ds_args;

ns_ret_t ns_create_ds (ns_create_ds_args args);

// Read srange from input data set to output file pointer
typedef struct
{
  const char *dbname;
  const char *dsname;
  FILE *fp;
  srange s;
} ns_ds_fp_args;

ns_ret_t ns_ds_fp (ns_ds_fp_args args);

// Read srange from input data set to output file pointer
typedef struct
{
  const char *dbname;
  const char *dsname;
  FILE *fp;
  srange s;
} ns_fp_ds_args;

ns_ret_t ns_fp_ds (ns_fp_ds_args args);
