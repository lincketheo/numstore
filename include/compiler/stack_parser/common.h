#pragma once

#include "compiler/tokens.h" // token
#include "type/types.h"      // type

// What you feed to the stack_parser
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
  SPR_NOMEM,
  SPR_SYNTAX_ERROR,
  SPR_DONE,

} stackp_result;
