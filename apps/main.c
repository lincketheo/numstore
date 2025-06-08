
#include "ast/query/query.h"
#include "ast/type/types.h"
#include "database.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "virtual_machine.h"

int
main (void)
{
  struct_t st;
  st.len = 4;
  st.keys = (string[]){
    {
        .len = 3,
        .data = "foo",
    },
    {
        .len = 2,
        .data = "fo",
    },
    {
        .len = 4,
        .data = "baro",
    },
    {
        .len = 5,
        .data = "bazbi",
    },
  };
  st.types = (type[]){
    {
        .type = T_PRIM,
        .p = U32,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = CF128,
    },
  };

  type t1 = {
    .type = T_STRUCT,
    .st = st,
  };

  error e = error_create (NULL);
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
  err_t_log_swallow (db_execute (db, &q1, &e), e);
  err_t_log_swallow (db_execute (db, &q1, &e), e);
  err_t_log_swallow (db_execute (db, &q1, &e), e);

  db_close (db);
}
