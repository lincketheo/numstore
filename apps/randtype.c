#include "ast/type/types.h"
#include "ast/type/types/prim.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/types.h"
#include "mm/lalloc.h"

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
