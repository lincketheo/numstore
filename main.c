#include "ns_usecases.h"

int
main ()
{
  /**
  ns_create_db_args cargs = {
    .dbname = "foo",
  };
  ns_create_ds_args dsargs = {
    .dbname = "foo",
    .dsname = "bar",
  };
  ns_create_db(cargs);
  ns_create_ds(dsargs);
  exit(0);
  */
  ns_fp_ds_args args = {
    .s = (srange){
             .start = 0,
             .isinf = 1,
             .stride = 1,
         },
    .fp = fopen("./foo.txt", "r"),
    .dbname = "foo",
    .dsname = "bar",
  };

  return ns_fp_ds (args);
}
