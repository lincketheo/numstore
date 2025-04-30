#pragma once

#include "compiler/type_parser.h"
#include "ds/cbuffer.h"
#include "intf/mm.h"

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *query_output;

  type_parser tp;
} parser;

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *query_output;
  tp_params tparams;
} parser_params;

void parser_create (parser *dest);

void parser_execute (parser *dest);

void parser_release (parser *dest);
