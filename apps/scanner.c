
#include "compiler/scanner.h"
#include "compiler/tokens.h"
#include "ds/cbuffer.h"
#include "intf/mm.h"
int
main ()
{
  // const char *data = "create a u32 ;; ; 123 123.4 123create 90.0 bit bool hello world";

  char _input[20];
  token _output[2];

  cbuffer input = cbuffer_create ((u8 *)_input, sizeof (_input));
  cbuffer output = cbuffer_create ((u8 *)_output, sizeof (_output));
  lalloc alloc;
  lalloc_create (&alloc, 1000);

  scanner_params params = {
    .chars_input = &input,
    .tokens_output = &output,
    .string_allocator = &alloc,
  };

  scanner s;
  scanner_create (&s, params);

  scanner_execute (&s);

  scanner_release (&s);
}
