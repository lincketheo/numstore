#include "variables/variable.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (var_hash_entry, var_hash_entry, v)
{
  ASSERT (v);
  ASSERT (v->vstr.len > 0);
  ASSERT (v->tlen > 0);
  ASSERT (v->tstr);
  ASSERT (v->vstr.data);
  ASSERT (v->pg0 != 0);
}

static const char *TAG = "Variable";

err_t
var_hash_entry_create (
    var_hash_entry *dest,
    const variable *src,
    lalloc *alloc,
    error *e)
{
  ASSERT (dest);
  ASSERT (src);
  ASSERT (src->vname.len > 0);
  ASSERT (src->vname.data);

  // Get the output buffer size for type string
  u32 tlen = type_get_serial_size (&src->type);
  ASSERT (tlen > 0);

  u8 *str = lmalloc (alloc, tlen + src->vname.len, 1);
  if (str == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate memory for var_hash_entry", TAG);
    }

  // Create the serialized data
  serializer s = srlizr_create (str, tlen);
  type_serialize (&s, &src->type);

  // Copy the variable string name
  i_memcpy (str + tlen, src->vname.data, src->vname.len);

  dest->vstr.data = (char *)(str + tlen);
  dest->vstr.len = src->vname.len;
  dest->tstr = str;
  dest->tlen = tlen;
  dest->pg0 = src->pg0;

  var_hash_entry_assert (dest);

  return SUCCESS;
}

err_t
var_hash_entry_deserialize (
    variable *dest,
    const var_hash_entry *src,
    lalloc *alloc,
    error *e)
{
  var_hash_entry_assert (src);

  // Allocate type
  type t;
  deserializer d = dsrlizr_create (src->tstr, src->tlen);
  err_t_wrap (type_deserialize (&t, &d, alloc, e), e);

  // Copy over variable name
  char *vname = lmalloc (alloc, src->vstr.len, 1);
  if (vname == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate memory for variable name", TAG);
    }
  i_memcpy (vname, src->vstr.data, src->vstr.len);

  // Set attributes
  dest->pg0 = src->pg0;
  dest->type = t;
  dest->vname = (string){
    .data = vname,
    .len = src->vstr.len,
  };

  return SUCCESS;
}
