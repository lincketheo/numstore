#pragma once

#include "ast/value/values/enum.h"
#include "ds/strings.h"
#include "errors/error.h"

typedef struct
{
  string choice;
  bool chosen;
} enumv_builder;

err_t envb_create (enumv_builder *dest);
err_t envb_accept_choice (enumv_builder *eb, string key);
err_t envb_build (enum_v *dest, enumv_builder *eb);
