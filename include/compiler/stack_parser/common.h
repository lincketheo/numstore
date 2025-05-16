#pragma once

#include "ast/type/types.h"  // type
#include "compiler/tokens.h" // token
#include "errors/error.h"    // ERR_...

// What you can get out of the stack_parser
typedef enum
{
  SBBT_TYPE,
  SBBT_QUERY,
} sb_build_type;

// What you can "give" to the stack_parser
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
  // Errors
  SPR_NOMEM = ERR_NOMEM,
  SPR_SYNTAX_ERROR = ERR_INVALID_ARGUMENT,

  // Status codes
  SPR_CONTINUE = 1,
  SPR_DONE = 2,
} stackp_result;
