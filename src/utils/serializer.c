#include "utils/serializer.h"
#include "dev/assert.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (serializer, serializer, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->dcap > 0);
  ASSERT (s->dlen <= s->dcap);
}

serializer
srlizr_create (u8 *data, u32 dcap)
{
  return (serializer){
    .data = data,
    .dlen = 0,
    .dcap = dcap,
  };
}

bool
srlizr_write (serializer *dest, const u8 *src, u32 len)
{
  serializer_assert (dest);

  if (dest->dlen + len > dest->dcap)
    {
      return false;
    }
  i_memcpy (dest->data + dest->dlen, src, len);
  dest->dlen += len;

  serializer_assert (dest);

  return true;
}
