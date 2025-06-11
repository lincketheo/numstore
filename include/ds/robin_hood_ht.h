#pragma once

#include "errors/error.h"
#include "intf/types.h"

typedef pgno hkey;

typedef struct
{
  hkey key;  // Hash key
  u32 index; // Index into an array that holds the actual values
} hdata;

typedef struct
{
  hdata data;   // The data we store
  u32 dib;      // Distance from initial bucket
  bool present; // Exists or not
} hentry;

typedef struct
{
  u32 cap;
  hentry elems[];
} hash_table;

hash_table *ht_open (u32 nelem, error *e);
void ht_close (hash_table *ht);

// Hash table insert result
typedef enum
{
  HTIR_SUCCESS,
  HTIR_EXISTS,
  HTIR_FULL,
} hti_res;

// Hash table access result
typedef enum
{
  HTAR_SUCCESS,
  HTAR_DOESNT_EXIST,
} hta_res;

hti_res ht_insert (hash_table *ht, hdata data);
hta_res ht_get (const hash_table *ht, hdata *dest, hkey key);
hta_res ht_delete (hash_table *ht, hkey key);
void i_log_ht (const hash_table *ht);
