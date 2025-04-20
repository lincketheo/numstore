#include "database.h"
#include "dev/errors.h"
#include "sds.h"

int
main (void)
{
  err_t ret = db_create (unsafe_cstrfrom ("test.db"), 2048, 1000);
  return ret;
}
