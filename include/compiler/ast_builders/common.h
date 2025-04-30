#pragma once

#include "compiler/tokens.h"

typedef enum
{

  PR_CONTINUE,
  PR_MALLOC_ERROR,
  PR_SYNTAX_ERROR,
  PR_DONE,

} parse_result;

typedef enum
{
  PE_TOKEN,
  PE_TYPE,
} parse_expect;
