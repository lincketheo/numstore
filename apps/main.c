#include "numstore.h"

int
main ()
{
  ns_create_db ((ns_create_db_args){
      .dbname = "foo",
  });
  ns_create_ds ((ns_create_ds_args){
      .dbname = "foo",
      .dsname = "bar",
  });
  return 0;
}
