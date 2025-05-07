#include "services/var_create.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "query/queries/create.h"
#include "variables/vmem_hashmap.h"

DEFINE_DBG_ASSERT_I (var_create, var_create, v)
{
  ASSERT (v);
  ASSERT (v->hm);
}

var_create
var_create_create (vcreate_params params)
{
  var_create ret;
  ret.hm = params.hm;
  var_create_assert (&ret);
  return ret;
}

err_t
var_create_create_var (var_create *v, create_query cargs)
{
  var_create_assert (v);

  err_t ret = SUCCESS;

  i_log_info ("Creating variable: %.*s\n", cargs.vname.len, cargs.vname.data);

  // This will be build using the pager
  vmeta mock = {
    .pgn0 = 1,
    .type = cargs.type,
  };

  if ((ret = vmhm_insert (v->hm, cargs.vname, mock)))
    {
      return ret;
    }

  i_log_info ("Create variable success. PGN0: %" PRIu64 "\n", mock.pgn0);

  return ret;
}
