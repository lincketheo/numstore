#include "database.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/stdlib.h"

int
main ()
{
  // Create database
  dbcargs cargs = {
    .fname = unsafe_cstrfrom ("test.db"),
    .page_size = 4096,
    .mpgr_len = 100,
  };

  database db;

  err_t ret;
  if ((ret = db_create (&db, cargs)))
    {
      switch (ret)
        {
        case SUCCESS:
        case ERR_ALREADY_EXISTS:
          break;
        default:
          return ret;
        }
    }

  // Open database
  dboargs args = {
    .fname = unsafe_cstrfrom ("test.db"),
  };

  if ((ret = db_open (&db, args)))
    {
      return ret;
    }

  db_close (&db);

  return 0;
}
