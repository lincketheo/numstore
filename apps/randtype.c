#include <stdio.h>  // TODO
#include <stdlib.h> // TODO
#include <time.h>   // TODO

#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO
#include "core/intf/types.h"   // TODO
#include "core/mm/lalloc.h"    // TODO

#include "compiler/ast/type/types.h"      // TODO
#include "compiler/ast/type/types/prim.h" // TODO

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
