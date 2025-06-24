#pragma once

#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO

#include "compiler/ast/value/values/enum.h" // TODO

typedef struct
{
  string choice;
  bool chosen;
} enumv_builder;

err_t envb_create (enumv_builder *dest);
err_t envb_accept_choice (enumv_builder *eb, string key);
err_t envb_build (enum_v *dest, enumv_builder *eb);
