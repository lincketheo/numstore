#include "navigation.h"
#include "database.h"
#include "intf/mm.h"
#include "paging.h"
#include "sds.h"

int
main ()
{
  // Open database
  if (db_open (unsafe_cstrfrom ("test.db")))
    {
      return -1;
    }

  global_database *db = get_global_database ();
  if (!db)
    {
      return -1;
    }

  navigator nav;
  lalloc alloc = lalloc_create (0);
  nav_create (&nav, &db->pager, &alloc);

  return 0;
}
