#include "op.h"
#include "intf/stdlib.h"
#include "paging.h"
#include "sds.h"
#include "typing.h"

DEFINE_DBG_ASSERT_H (create, create, c)
{
  ASSERT (c);
  string_assert (&c->vname);
  type_assert (c->type);
  pager_assert (c->p);
}

void
create_init (create *c, const string vname, const type *type, pager *p)
{
  ASSERT (c);
  string_assert (&vname);
  type_assert (type);
  pager_assert (p);

  create _c = (create){
    .vname = vname,
    .type = type,
    .p = p,
  };
  i_memcpy (c, &_c, sizeof _c);
}

void
create_execute (create *c)
{
  create_assert (c);
}
