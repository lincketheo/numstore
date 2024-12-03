#pragma once

#include <stdio.h>

///////////////////////////////
////////// Buffers

typedef struct
{
  void *data;
  size_t cap;
  size_t len;
} u8_qbuf;

typedef struct
{
  void *data;
  size_t cap;
  size_t len;
} u8_sbuf;

///////////////////////////////
////////// srange

typedef enum
{
  U8 = 0,
  I8 = 1,

  U16 = 2,
  I16 = 3,

  U32 = 4,
  I32 = 5,

  U64 = 6,
  I64 = 7,

  F16 = 8,
  F32 = 9,
  F64 = 10,

  CF32 = 11,
  CF64 = 12,
  CF128 = 13,

  CI16 = 14,
  CI32 = 15,
  CI64 = 16,
  CI128 = 17,

  CU16 = 18,
  CU32 = 19,
  CU64 = 20,
  CU128 = 21,
} dtype;

#define dtype_max 20

size_t dtype_sizeof (dtype d);

///////////////////////////////
////////// srange

typedef struct
{
  size_t start;
  size_t end;
  size_t stride;
  int isinf;
} srange;

#define srange_ASSERT(s)                                                      \
  ASSERT (s.start <= s.end);                                                  \
  ASSERT (s.stride > 0)

///////////////////////////////
////////// Database

// Create Database
typedef struct
{
  const char *dbname;
} ns_create_db_args;

int ns_create_db (ns_create_db_args args);

// Create Dataset
typedef struct
{
  const char *dbname;
  const char *dsname;
} ns_create_ds_args;

int ns_create_ds (ns_create_ds_args args);

// Read srange from input data set to output file pointer
typedef struct
{
  const char *dbname;
  const char *dsname;
  FILE *fp;
  srange s;
} ns_read_srange_ds_fp_args;

int ns_read_srange_ds_fp (ns_read_srange_ds_fp_args args);

///////////////////////////////
////////// Error handling

#define fail() (*(volatile int *)0 = 0)

typedef struct
{
  int _errno;
} ns_error;

void ns_error_report (ns_error e);

ns_error ns_error_errno (int _errno);

#define unreachable()                                                         \
  do                                                                          \
    {                                                                         \
      fprintf (stderr,                                                        \
               "BUG! Unreachable! "                                           \
               "(%s):%s:%d\n",                                                \
               __FILE__, __func__, __LINE__);                                 \
      fail ();                                                                \
    }                                                                         \
  while (0)

#ifndef NDEBUG

#define ASSERT(expr)                                                          \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          fprintf (stderr,                                                    \
                   "BUG! ASSERTION: " #expr " failed. "                       \
                   "Location: (%s):%s:%d\n",                                  \
                   __FILE__, __func__, __LINE__);                             \
          fail ();                                                            \
        }                                                                     \
    }                                                                         \
  while (0)

#define TODO(msg)                                                             \
  do                                                                          \
    {                                                                         \
      fprintf (stderr,                                                        \
               "TODO! " msg " "                                               \
               "Location: (%s):%s:%d\n",                                      \
               __FILE__, __func__, __LINE__);                                 \
      fail ();                                                                \
    }                                                                         \
  while (0)

#else

#define ASSERT(expr)

#endif
