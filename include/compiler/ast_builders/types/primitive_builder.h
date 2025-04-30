#pragma once

#include "compiler/ast_builders/common.h"
#include "typing.h"

typedef struct type_builder_s type_builder;

parse_result prim_create (type_builder *tb, prim_t p);
