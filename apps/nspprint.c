
#include "nsutils/page_print.h"
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  if (argc != 3 && argc != 2)
    {
      printf ("USAGE: nspprint FNAME [PGNO]\n");
    }

  int pgno;
  if (argc == 2)
    {
      pgno = -1;
    }
  else
    {
      pgno = atoi (argv[2]);
    }
  char *fname = argv[1];

  page_print (fname, pgno);

  return 0;
}
