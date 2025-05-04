#include "compiler/token_printer.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "intf/logging.h"

DEFINE_DBG_ASSERT_I (token_printer, token_printer, t)
{
  ASSERT (t);
}

void
tokp_create (token_printer *dest, tokp_params params)
{
  ASSERT (dest);
  ASSERT (params.tokens_inputs->cap % sizeof (token) == 0);

  dest->tokens_inputs = params.tokens_inputs;
  dest->strings_deallocator = params.strings_deallocator;
}

void
tokp_execute (token_printer *t)
{
  token_printer_assert (t);

  token tok;
  u32 read = cbuffer_read (&tok, sizeof tok, 1, t->tokens_inputs);
  ASSERT (read == 0 || read == 1);

  if (read > 0)
    {
      const string cstr = tt_tostr (tok.type);
      ASSERT (cstr.len <= MAX_TOK_T_LEN);
      i_log_info ("Got token: %.*s\n",
                  tt_tostr (tok.type).len, tt_tostr (tok.type).data);

      // Cleanup
      switch (tok.type)
        {
        case TT_IDENTIFIER:
        case TT_STRING:
          {
            lfree (t->strings_deallocator, tok.str.data);
            break;
          }
        default:
          break;
        }
    }
}
