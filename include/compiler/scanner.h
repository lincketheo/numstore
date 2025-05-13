#pragma once

#include "stmtctrl.h" // stmtctrl

#include "ds/cbuffer.h" // cbuffer
#include "intf/types.h" // u32

////////////////////// SCANNER (chars -> tokens)

typedef enum
{
  SS_START,
  SS_IDENT,
  SS_STRING,
  SS_NUMBER,
  SS_DECIMAL,
} scanner_state;

typedef struct
{
  scanner_state state;

  cbuffer *chars_input;
  cbuffer *tokens_output;

  /**
   * "Scratch space". Contains:
   *    - Internal growing string for
   *      identifiers / floats / strings
   */
  char str[512];
  u32 slen;

  stmtctrl *ctrl;
} scanner;

scanner scanner_create (
    cbuffer *chars_input,
    cbuffer *tokens_output,
    stmtctrl *ctrl);

void scanner_execute (scanner *s);
