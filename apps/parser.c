
#include "compiler/parser.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "mm/lalloc.h"

#include <stdlib.h>

int
main ()
{
  void *p = parse_alloc ();

  u8 data[4096];

  lalloc alloc = lalloc_create (data, 4096);
  error e = error_create (NULL);

  parser_ctxt ctx = pctx_create (&alloc, &alloc);

  if (parse (p, quick_tok (TT_STRUCT), &ctx, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (parse (p, quick_tok (TT_LEFT_BRACE), &ctx, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (parse (p, tt_ident (unsafe_cstrfrom ("foo")), &ctx, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (parse (p, tt_prim (U32), &ctx, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  if (parse (p, quick_tok (TT_RIGHT_BRACE), &ctx, &e))
    {
      error_log_consume (&e);
      return -1;
    }

  parse_free (p);

  return 0;
}
