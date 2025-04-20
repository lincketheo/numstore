#include "database.h"
#include "dev/errors.h"
#include "sds.h"

int
main (void)
{
  err_t ret = db_open (unsafe_cstrfrom ("test.db"));
  return ret;
}
