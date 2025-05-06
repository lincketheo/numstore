#include "utils/deserializer.h"
#include "dev/assert.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (deserializer, deserializer, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->dlen > 0);
  ASSERT (s->head <= s->dlen);
}

deserializer
dsrlizr_create (u8 *data, u32 dlen)
{
  return (deserializer){
    .data = data,
    .head = 0,
    .dlen = dlen,
  };
}

bool
dsrlizr_read (u8 *dest, u32 dlen, deserializer *src)
{
  deserializer_assert (src);

  if (src->head + dlen > src->dlen)
    {
      return false;
    }
  i_memcpy (dest, src->data + src->head, dlen);
  src->head += dlen;

  deserializer_assert (src);

  return true;
}
