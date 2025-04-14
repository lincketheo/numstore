#include "eventloop.h"

int
main (void)
{
  setup ();

  while (1)
    {
      execute ();
    }

  teardown ();

  return 0;
}
