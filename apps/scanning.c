#include "compiler.h"
#include "intf/mm.h"
#include "sds.h"
#include <string.h>

token_printer t;
const char *stmt = "write ; ; 1.123 100 hello write +10091.1 ";
int i = 0;
char chars_backing[10];
token tokens_backing[10];

cbuffer chars;
cbuffer tokens;

scanner s;
token_printer t;

void
feed_execute (void)
{
  if ((u32)i < strlen (stmt))
    {
      i += cbuffer_write (&stmt[i], 1, strlen (&stmt[i]), &chars);
    }
}

int
main (void)
{
  stdalloc alloc = stdalloc_create (0);
  chars = cbuffer_create ((u8 *)chars_backing, 10);
  tokens = cbuffer_create ((u8 *)tokens_backing, 10 * sizeof *tokens_backing);
  scanner_create (&s, &chars, &tokens, &alloc.alloc);
  token_printer_create (&t, &tokens);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);

  feed_execute ();
  scanner_execute (&s);
  token_printer_execute (&t);
}
