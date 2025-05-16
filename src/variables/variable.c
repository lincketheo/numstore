#include "variables/variable.h"
#include "dev/assert.h"

DEFINE_DBG_ASSERT_I (var_hash_entry, var_hash_entry, v)
{
  ASSERT (v);
  ASSERT (v->vlen > 0);
  ASSERT (v->tlen > 0);
  ASSERT (v->tstr);
  ASSERT (v->vstr);
  ASSERT (v->pg0 != 0);
}

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

  /**
   * Get the output buffer size for
   * type string
   */
  u32 tlen = type_byte_size (&src->type);
  ASSERT (tlen > 0);

  void *tstr = lmalloc (alloc, tlen, 1);
  if (tstr == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Failed to allocate type string");
    }

  serializer s = srlizr_create (tstr, tlen);
  type_serialize (&s, &src->type);

  dest->vstr = src->vname.data;
  dest->vlen = src->vname.len;
  dest->tstr = tstr;
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

  type t;
  deserializer d = dsrlizr_create (src->tstr, src->tlen);
  err_t_wrap (type_deserialize (&t, &d, alloc, e), e);

  dest->pg0 = src->pg0;
  dest->type = t;
  dest->vname = (string){
    .data = src->vstr,
    .len = src->vlen,
  };

  return SUCCESS;
}
