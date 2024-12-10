#include "ns_memory.h"
#include <stdio.h>

int
main ()
{
  ns_alloc s;
  fprintf (stderr, "%d\n", limited_linalloc_create (&s, 10));

  bytes b;
  fprintf (stderr, "%d\n", s.ns_malloc (&s, &b, 10));

  fprintf(stderr, "%zu\n", b.cap);

  return 0;
}
