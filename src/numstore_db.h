#pragma once

#include "numstore_dtype.h"
#include <stdio.h>

//////////////////////////////////////////////////
/////////// CREATE DATABASE

typedef struct
{
  const char *dbname;
} nscdb_args;

typedef struct
{
  enum
  {
    NSCDB_RT_SUCCESS,
    NSCDB_RT_FILE_CONFLICT,
    NSCDB_RT_OS_FAILURE,
  } type;
  int errno_from_os_failure;
} nscdb_ret;

void nscdb_log (nscdb_args args, nscdb_ret ret);

nscdb_ret nscdb (nscdb_args args);

//////////////////////////////////////////////////
/////////// CREATE DATASET

typedef struct
{
  const char *dbname;
  const char *dsname;
  dtype type;
} nscds_args;

typedef struct
{
  enum
  {
    NSCDS_RT_SUCCESS,
    NSCDS_RT_DB_DOESNT_EXIST,
    NSCDS_RT_CONFLICT,
    NSCDS_RT_NAMES_TOO_LONG,
    NSCDS_RT_OS_FAILURE,
  } type;
  int errno_from_os_failure;
} nscds_ret;

void nscds_log (nscds_args args, nscds_ret ret);

nscds_ret nscds (nscds_args args);

//////////////////////////////////////////////////
/////////// CREATE DATASET

int nswds_fp (const char *dbname, const char *dsname, FILE *rfp);

// Read from dataset
int nsrds_fp (const char *dbname, const char *dsname, FILE *wfp);

// Delete dataset
int nsdds (const char *dbname, const char *dsname);
