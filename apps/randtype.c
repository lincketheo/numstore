#include "compiler/ast/type/types.h"
#include "compiler/ast/type/types/prim.h"
#include "core/errors/error.h"
#include "core/intf/io.h"
#include "core/intf/types.h"
#include "core/mm/lalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int
main (void)
{
  srand (time (NULL));
  type t;
  u8 buf[2048];
  lalloc l = lalloc_create (buf, 2048);
  error e = error_create (NULL);
  if (type_random (&t, &l, 5, &e))
    {
      fprintf (stdout, "Failed to create random type\n");
      return -1;
    }

  i_log_type (t, &e);

  return 0;
}
