#pragma once

#include "compiler/stack_parser/common.h"
#include "type/types.h"

typedef struct type_builder_s type_builder;

stackp_result prim_create (type_builder *tb, prim_t p);
