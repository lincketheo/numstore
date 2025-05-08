#pragma once

#include "compiler/stack_parser/common.h"
#include "type/types.h"

typedef struct type_parser_s type_parser;

stackp_result prim_create (type_parser *tb, prim_t p);
