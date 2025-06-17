
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

  u8 data[4096];

  error e = error_create (NULL);

  query_provider *q = query_provider_create (&e);
  if (q == NULL)
    {
      error_log_consume (&e);
      return -1;
    }

  query c;
  if (query_provider_get (q, &c, QT_CREATE, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  token create = (token){
    .type = TT_CREATE,
    .q = c,
  };
  /*
  if (query_provider_get (q, &c, QT_DELETE, &e))
    {
      error_log_consume (&e);
      return -1;
    }
  token delete = (token){
    .type = TT_DELETE,
    .q = c,
  };
  */

  lalloc work = lalloc_create (data, 4096);
  parser_result ctx;
  err_t_wrap (parser_create (&ctx, &work, c.qalloc, &e), &e);

  /*
  token tokens[] = {
    delete,
    tt_ident (unsafe_cstrfrom ("foobar")),
    quick_tok (0),
  };
  */

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
    quick_tok (TT_SEMICOLON),
  };

  u32 i = 0;
  while (!ctx.ready)
    {
      ASSERT (i < arrlen (tokens));

      if (parser_parse (&ctx, tokens[i], &e))
        {
          error_log_consume (&e);
          return -1;
        }

      i++;
    }

  query _c = parser_consume (&ctx);
  i_log_query (_c);

  return 0;
}
