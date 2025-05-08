#include "services/var_retr.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "variables/vmem_hashmap.h"

DEFINE_DBG_ASSERT_I (var_retr, var_retr, v)
{
  ASSERT (v);
  ASSERT (v->hm);
}

var_retr
var_retr_create (vrtr_params params)
{
  var_retr ret;
  ret.hm = params.hm;
  var_retr_assert (&ret);
  return ret;
}

err_t
var_retr_get_var (var_retr *v, vmeta *dest, const string vname)
{
  var_retr_assert (v);
  (void)vname;
  (void)dest;
  return SUCCESS; // vmhm_get (v->hm, dest, vname);
}
