#include "variables/variable.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "type/types.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

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
vm_to_vhe (
    var_hash_entry *dest,
    string vname,
    vmeta src,
    lalloc *tstr_allocator)
{
  ASSERT (dest);
  ASSERT (vname.len > 0);
  ASSERT (vname.data);

  /**
   * Get the output buffer size for
   * type string
   */
  u32 tlen = type_byte_size (&src.type);
  ASSERT (tlen > 0);

  /**
   * Allocate tstr
   */
  u8 *tstr = lmalloc (tstr_allocator, tlen);
  if (!tstr)
    {
      return ERR_NOMEM;
    }
  serializer s = srlizr_create (tstr, tlen);
  type_serialize (&s, &src.type);

  dest->vstr = vname.data;
  dest->vlen = vname.len;
  dest->tstr = tstr;
  dest->tlen = tlen;
  dest->pg0 = src.pgn0;

  var_hash_entry_assert (dest);

  return SUCCESS;
}

void
var_hash_entry_free (var_hash_entry *v, lalloc *tstr_allocator)
{
  lfree (tstr_allocator, v->tstr);
}

err_t
vhe_to_vm (
    vmeta *dmeta,
    const var_hash_entry *src,
    lalloc *type_allocator)
{
  ASSERT (dmeta);
  var_hash_entry_assert (src);

  type t;
  deserializer d = dsrlizr_create (src->tstr, src->tlen);
  err_t ret = type_deserialize (&t, &d, type_allocator);
  if (ret)
    {
      return ret;
    }

  dmeta->pgn0 = src->pg0;
  dmeta->type = t;

  return SUCCESS;
}
