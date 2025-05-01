#pragma once

#include "compiler/tokens.h"

typedef enum
{
  SBFT_TOKEN,
  SBFT_TYPE,
} sb_feed_t;

typedef struct
{
  union
  {
    token tok;
    type type;
  };
} sb_feed;

// The result of one feed result
typedef enum
{

  SPR_CONTINUE,
  SPR_MALLOC_ERROR,
  SPR_SYNTAX_ERROR,
  SPR_DONE,

} stackp_result;
