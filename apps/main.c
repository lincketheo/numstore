
#include "ast/query/query.h"
#include "ast/type/types.h"
#include "database.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "virtual_machine.h"

#include <stdlib.h>
#include <time.h>

int
main (void)
{
  srand (time (NULL));
  u8 buf[20480];
  lalloc l = lalloc_create (buf, 2048);
  error e = error_create (NULL);

  type t1, t2, t3;
  if (type_random (&t1, &l, 5, &e))
    {
      i_log_error ("Failed to create random type\n");
      return -1;
    }
  if (i_log_type (t1, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (type_random (&t2, &l, 5, &e))
    {
      i_log_error ("Failed to create random type\n");
      return -1;
    }
  if (i_log_type (t2, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (type_random (&t3, &l, 5, &e))
    {
      i_log_error ("Failed to create random type\n");
      return -1;
    }
  if (i_log_type (t3, &e))
    {
      error_log_consume (&e);
      return -1;
    }

  database *db = db_open (unsafe_cstrfrom ("test.db"), &e);
  if (db == NULL)
    {
      error_log_consume (&e);
      return -1;
    }

  create_query c1;

  query q1 = create_query_create (&c1);

  c1.vname = (string){
    .data = "foo",
    .len = 3,
  };
  c1.type = t1;

  err_t_log_swallow (db_execute (db, &q1, &e), e);

  c1.vname = (string){
    .data = "bar",
    .len = 3,
  };
  c1.type = t2;

  err_t_log_swallow (db_execute (db, &q1, &e), e);

  c1.vname = (string){
    .data = "biz",
    .len = 3,
  };
  c1.type = t3;

  err_t_log_swallow (db_execute (db, &q1, &e), e);

  db_close (db);
}
