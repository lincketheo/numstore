#pragma once

#include "ds/cbuffer.h"
#include "intf/mm.h"

typedef struct
{
  cbuffer *tokens_inputs;
  lalloc *strings_deallocator;
} token_printer;

typedef struct
{
  cbuffer *tokens_inputs;
  lalloc *strings_deallocator;
} tokp_params;

void tokp_create (token_printer *dest, tokp_params params);
void tokp_execute (token_printer *t);
