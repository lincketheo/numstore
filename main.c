
#include <stdio.h>

typedef union
{
  int a;
  char b;
} foo;

int
main ()
{
  foo a = {
    .a = 5,
    .b = 10,
  };
  fprintf (stdout, "%d\n", a.a);
  return 0;
}
