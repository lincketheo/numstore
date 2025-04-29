#pragma once

#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/types.h"

typedef struct
{
  char *buffer;
  u32 blen;
} repl;

void repl_create (repl *dest);

err_t repl_read (repl *r);

err_t repl_execute (repl *r);
