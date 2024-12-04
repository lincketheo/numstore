#include "ns_usecases.h"

int
main ()
{
  ns_create_ds_args args = {
    .dbname = "foo",
    .dsname = "bar",
  };

  return ns_create_ds (args);
}
