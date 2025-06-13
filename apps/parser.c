
#include "compiler/parser.h"
#include "ast/query/queries/create.h"
#include "ast/query/query.h"
#include "ast/query/query_provider.h"
#include "compiler/tokens.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "mm/lalloc.h"
#include "utils/macros.h"

#include <stdlib.h>

int
main (void)
{
  void *p = parse_alloc ();

  u8 data[4096];

  error e = error_create (NULL);

  query_provider *q = query_provider_create (&e);
  if (q == NULL)
    {
      error_log_consume (&e);
      return -1;
    }

  query c;
  query_provider_get (q, &c, QT_CREATE, &e);
  token create = (token){
    .type = TT_CREATE,
    .q = c,
  };

  lalloc work = lalloc_create (data, 4096);
  parser_ctxt ctx = pctx_create (&work, c.qalloc);

  token tokens[] = {
    create,
    tt_ident (unsafe_cstrfrom ("a")),

    // struct {
    quick_tok (TT_STRUCT),
    quick_tok (TT_LEFT_BRACE),

    // foo U32,
    tt_ident (unsafe_cstrfrom ("foo")),
    tt_prim (U32),
    quick_tok (TT_COMMA),

    // bar enum { bar, biz, buz, }, // notice trailing comma
    tt_ident (unsafe_cstrfrom ("bar")),
    quick_tok (TT_ENUM),
    quick_tok (TT_LEFT_BRACE),
    tt_ident (unsafe_cstrfrom ("bar")),
    quick_tok (TT_COMMA),
    tt_ident (unsafe_cstrfrom ("biz")),
    quick_tok (TT_COMMA),
    tt_ident (unsafe_cstrfrom ("buz")),
    quick_tok (TT_RIGHT_BRACE),
    quick_tok (TT_COMMA),

    // biz struct { a U32, b union { c i32, d u64 } },
    tt_ident (unsafe_cstrfrom ("biz")),
    quick_tok (TT_STRUCT),
    quick_tok (TT_LEFT_BRACE),
    tt_ident (unsafe_cstrfrom ("a")),
    tt_prim (U32),
    quick_tok (TT_COMMA),
    tt_ident (unsafe_cstrfrom ("b")),
    quick_tok (TT_UNION),
    quick_tok (TT_LEFT_BRACE),
    tt_ident (unsafe_cstrfrom ("c")),
    tt_prim (I32),
    quick_tok (TT_COMMA),
    tt_ident (unsafe_cstrfrom ("d")),
    tt_prim (U64),
    quick_tok (TT_RIGHT_BRACE),
    quick_tok (TT_RIGHT_BRACE),
    quick_tok (TT_COMMA),

    // baz [10][20][30] struct { i i32 }
    tt_ident (unsafe_cstrfrom ("baz")),
    quick_tok (TT_LEFT_BRACKET),
    tt_integer (10),
    quick_tok (TT_RIGHT_BRACKET),
    quick_tok (TT_LEFT_BRACKET),
    tt_integer (20),
    quick_tok (TT_RIGHT_BRACKET),
    quick_tok (TT_LEFT_BRACKET),
    tt_integer (30),
    quick_tok (TT_RIGHT_BRACKET),
    quick_tok (TT_STRUCT),
    quick_tok (TT_LEFT_BRACE),
    tt_ident (unsafe_cstrfrom ("i")),
    tt_prim (I32),
    quick_tok (TT_RIGHT_BRACE),

    // }
    quick_tok (TT_RIGHT_BRACE),
    quick_tok (0),
  };

  for (u32 i = 0; i < arrlen (tokens); ++i)
    {
      if (parse (p, tokens[i], &ctx, &e))
        {
          error_log_consume (&e);
          return -1;
        }
    }

  parse_free (p);

  return 0;
}
