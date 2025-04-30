#include "compiler/token_printer.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"

DEFINE_DBG_ASSERT_I (token_printer, token_printer, t)
{
  ASSERT (t);
}

void
tokp_create (token_printer *dest, tokp_params params)
{
  ASSERT (dest);
  ASSERT (params.tokens_inputs->cap % sizeof (token) == 0);
  ASSERT (params.chars_output->cap >= MAX_TOK_T_LEN);

  dest->tokens_inputs = params.tokens_inputs;
  dest->chars_output = params.chars_output;
  dest->strings_deallocator = params.strings_deallocator;
}

void
tokp_execute (token_printer *t)
{
  token_printer_assert (t);

  if (cbuffer_avail (t->chars_output) < MAX_TOK_T_LEN)
    {
      return;
    }

  token tok;
  u32 read = cbuffer_read (&tok, sizeof tok, 1, t->tokens_inputs);
  ASSERT (read == 0 || read == 1);

  if (read > 0)
    {
      const string cstr = tt_tostr (tok.type);
      ASSERT (cstr.len <= MAX_TOK_T_LEN);
      cbuffer_write (cstr.data, 1, cstr.len, t->chars_output);

      // Cleanup
      switch (tok.type)
        {
        case TT_IDENTIFIER:
          lfree (t->strings_deallocator, tok.str.data);
          break;
        default:
          break;
        }
    }
}
