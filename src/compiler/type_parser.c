#include "compiler/type_parser.h"
#include "compiler/tokens.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "utils/bounds.h"

////////////////// TYPE BUILDER
/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> V | S
 * K -> \eps | , T K
 * V -> [] V | []
 * S -> [NUM] S | [NUM]
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * V = VARRAY_BRACKETS
 * S = SARRAY_BRACKETS
 * I = ENUM_KEY_LIST
 */

typedef struct
{

  enum
  {
    SB_WAITING_FOR_LB,
    SB_WAITING_FOR_IDENT,
    SB_WAITING_FOR_TYPE,
    SB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} struct_bldr;

typedef struct
{

  enum
  {
    UB_WAITING_FOR_LB,
    UB_WAITING_FOR_IDENT,
    UB_WAITING_FOR_TYPE,
    UB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} union_bldr;

typedef struct
{

  enum
  {
    EB_WAITING_FOR_LB,
    EB_WAITING_FOR_IDENT,
    EB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  u32 len;
  u32 cap;

} enum_bldr;

typedef struct
{

  enum
  {
    VAB_WAITING_FOR_LEFT_OR_TYPE,
    VAB_WAITING_FOR_RIGHT,
    VAB_WAITING_FOR_TYPE,
  } state;

  u32 rank;

} varray_bldr;

typedef struct
{

  enum
  {
    SAB_WAITING_FOR_LEFT_OR_TYPE,
    SAB_WAITING_FOR_NUMBER,
    SAB_WAITING_FOR_RIGHT,
    SAB_WAITING_FOR_TYPE,
  } state;

  u32 *dims;
  u32 len;
  u32 cap;

} sarray_bldr;

struct type_bldr_s
{

  enum
  {
    TB_UNKNOWN,
    TB_ARRAY_UNKNOWN,

    TB_STRUCT,
    TB_UNION,
    TB_ENUM,
    TB_VARRAY,
    TB_SARRAY,
    TB_PRIM,

  } state;

  union
  {
    struct_bldr sb;
    union_bldr ub;
    enum_bldr eb;
    varray_bldr vab;
    sarray_bldr sab;
  };

  type ret;
};

DEFINE_DBG_ASSERT_I (type_bldr, type_bldr, tb)
{
  ASSERT (tb);
}

////////////////////// TYPE PARSER

DEFINE_DBG_ASSERT_I (type_parser, type_parser, t)
{
  ASSERT (t);
  ASSERT (t->stack);
  ASSERT (t->sp > 0);
}

static inline type_bldr
tb_create (void)
{
  return (type_bldr){
    .state = TB_UNKNOWN,
  };
}

err_t
tp_create (type_parser *dest, tp_params params)
{
  ASSERT (dest);

  // Allocate the stack - start with 3 layers, which is pretty normal
  type_bldr *stack = lmalloc (
      params.stack_allocator,
      3 * sizeof *stack);

  if (!stack)
    {
      return ERR_NOMEM;
    }

  dest->stack = stack;
  dest->sp = 0;

  dest->type_allocator = params.type_allocator;
  dest->stack_allocator = params.stack_allocator;

  dest->stack[dest->sp++] = tb_create ();

  return SUCCESS;
}

//////////////// STRUCT BUILDER
static inline DEFINE_DBG_ASSERT_I (struct_bldr, struct_bldr, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);
}

static inline tp_result
stbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->sb.keys = lmalloc (alloc, 5 * sizeof *dest->sb.keys);
  if (!dest->sb.keys)
    {
      return TPR_MALLOC_ERROR;
    }
  dest->sb.types = lmalloc (alloc, 5 * sizeof *dest->sb.types);
  if (!dest->sb.types)
    {
      lfree (alloc, dest->sb.keys);
      return TPR_MALLOC_ERROR;
    }

  dest->sb.cap = 5;
  dest->sb.len = 0;
  dest->sb.state = SB_WAITING_FOR_LB;
  dest->state = TB_STRUCT;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
sb_handle_waiting_for_lb (type_bldr *sb, token t)
{
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  sb->sb.state = SB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

// Push a string to the end
static inline tp_result
stbldr_push_key (type_bldr *sb, token t, lalloc *alloc)
{
  struct_bldr_assert (&sb->sb);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (sb->sb.len == sb->sb.cap)
    {
      ASSERT (can_mul_u64 (sb->sb.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sb.cap, sizeof (string)));

      u32 ncap = 2 * sb->sb.cap;
      string *keys = lrealloc (alloc, sb->sb.keys, ncap * sizeof *keys);
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, sb->sb.types, ncap * sizeof *types);
      if (!keys)
        {
          lfree (alloc, keys);
          return TPR_MALLOC_ERROR;
        }
      sb->sb.keys = keys;
      sb->sb.types = types;
      sb->sb.cap = ncap;
    }

  sb->sb.keys[sb->sb.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  sb->sb.state = SB_WAITING_FOR_TYPE;

  return TPR_EXPECT_NEXT_TYPE;
}

static inline tp_result
sb_handle_comma_or_right (type_bldr *sb, token t)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      sb->sb.state = SB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }
  return TPR_SYNTAX_ERROR;
}

static inline tp_result
sb_accept_token (type_bldr *tb, token t, lalloc *alloc)
{
  type_bldr_assert (tb);
  ASSERT (tb->state == TB_STRUCT);

  switch (tb->sb.state)
    {
    case SB_WAITING_FOR_LB:
      {
        return sb_handle_waiting_for_lb (tb, t);
      }

    case SB_WAITING_FOR_IDENT:
      {
        return stbldr_push_key (tb, t, alloc);
      }

    case SB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return sb_handle_comma_or_right (tb, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
stbldr_push_type (type_bldr *sb, type t)
{
  type_bldr_assert (sb);
  ASSERT (sb->sb.len < sb->sb.cap); // From previous string push
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_TYPE);

  sb->sb.types[sb->sb.len++] = t; // NOW YOU CAN INC LEN
  sb->sb.state = SB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
sb_accept_type (type_bldr *sb, type type)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);

  switch (sb->sb.state)
    {
    case SB_WAITING_FOR_TYPE:
      {
        return stbldr_push_type (sb, type);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
sb_build (type_bldr *sb, lalloc *alloc)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_COMMA_OR_RIGHT);

  type *types = sb->sb.types;
  string *strs = sb->sb.keys;

  sb->ret.st = lmalloc (alloc, sizeof *sb->ret.st);
  if (!sb->ret.st)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (sb->sb.len < sb->sb.cap)
    {
      types = lrealloc (alloc, types, sb->sb.len * sizeof *types);
      if (!types)
        {
          return TPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, sb->sb.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  sb->ret.st->types = types;
  sb->ret.st->keys = strs;
  sb->ret.st->len = sb->sb.len;
  sb->ret.type = T_STRUCT;

  return TPR_DONE;
}

//////////////// UNION BUILDER
static inline DEFINE_DBG_ASSERT_I (union_bldr, union_bldr, u)
{
  ASSERT (u);
  ASSERT (u->keys);
  ASSERT (u->types);
  ASSERT (u->cap > 0);
  ASSERT (u->len <= u->cap);
}

static inline tp_result
unbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->ub.keys = lmalloc (alloc, 5 * sizeof *dest->ub.keys);
  if (!dest->ub.keys)
    {
      return TPR_MALLOC_ERROR;
    }

  dest->ub.types = lmalloc (alloc, 5 * sizeof *dest->ub.types);
  if (!dest->ub.types)
    {
      lfree (alloc, dest->ub.keys);
      return TPR_MALLOC_ERROR;
    }

  dest->ub.cap = 5;
  dest->ub.len = 0;
  dest->ub.state = UB_WAITING_FOR_LB;
  dest->state = TB_UNION;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
ub_handle_waiting_for_lb (type_bldr *ub, token t)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  ub->ub.state = UB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

// Push a key to the end
static inline tp_result
unbldr_push_key (type_bldr *ub, token t, lalloc *alloc)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (ub->ub.len == ub->ub.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (ub->ub.cap, 2));
      ASSERT (can_mul_u64 (2 * ub->ub.cap, sizeof (string)));
      u32 ncap = 2 * ub->ub.cap;
      string *keys = lrealloc (alloc, ub->ub.keys, ncap * sizeof (string));
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, ub->ub.types, ncap * sizeof (type));
      if (!keys)
        {
          lfree (alloc, keys);
          return TPR_MALLOC_ERROR;
        }
      ub->ub.keys = keys;
      ub->ub.types = types;
      ub->ub.cap = ncap;
    }

  ub->ub.keys[ub->ub.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  ub->ub.state = UB_WAITING_FOR_TYPE;

  return TPR_EXPECT_NEXT_TYPE;
}

static inline tp_result
ub_handle_comma_or_right (type_bldr *ub, token t)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      ub->ub.state = UB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }

  return TPR_SYNTAX_ERROR;
}

static inline tp_result
ub_accept_token (type_bldr *ub, token t, lalloc *alloc)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {

    case UB_WAITING_FOR_LB:
      {
        return ub_handle_waiting_for_lb (ub, t);
      }

    case UB_WAITING_FOR_IDENT:
      {
        return unbldr_push_key (ub, t, alloc);
      }

    case UB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return ub_handle_comma_or_right (ub, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
unbldr_push_type (type_bldr *ub, type t)
{
  type_bldr_assert (ub);
  ASSERT (ub->ub.len < ub->ub.cap);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_TYPE);

  ub->ub.types[ub->ub.len++] = t; // NOW YOU CAN INC LEN
  ub->ub.state = UB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
ub_accept_type (type_bldr *ub, type type)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {
    case UB_WAITING_FOR_TYPE:
      {
        return unbldr_push_type (ub, type);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
ub_build (type_bldr *ub, lalloc *alloc)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_COMMA_OR_RIGHT);

  type *types = ub->ub.types;
  string *strs = ub->ub.keys;

  ub->ret.un = lmalloc (alloc, sizeof *ub->ret.un);
  if (!ub->ret.un)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (ub->ub.len < ub->ub.cap)
    {
      types = lrealloc (alloc, types, ub->ub.len * sizeof *types);
      if (!types)
        {
          return TPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, ub->ub.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  ub->ret.un->types = types;
  ub->ret.un->keys = strs;
  ub->ret.un->len = ub->ub.len;
  ub->ret.type = T_UNION;

  return TPR_DONE;
}

//////////////// ENUM BUILDER
DEFINE_DBG_ASSERT_I (enum_bldr, enum_bldr, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);
}

static inline tp_result
enbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->eb.keys = lmalloc (alloc, 5 * sizeof *dest->eb.keys);
  if (!dest->eb.keys)
    {
      return TPR_MALLOC_ERROR;
    }

  dest->eb.cap = 5;
  dest->eb.len = 0;
  dest->eb.state = EB_WAITING_FOR_LB;
  dest->state = TB_ENUM;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
eb_handle_waiting_for_lb (type_bldr *eb, token t)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  eb->eb.state = EB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
enbldr_push_str (type_bldr *eb, token t, lalloc *alloc)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);
  ASSERT (eb->eb.state == EB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }

  // Check for size adjustments
  if (eb->eb.len == eb->eb.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (eb->eb.cap, 2));
      ASSERT (can_mul_u64 (2 * eb->eb.cap, sizeof (string)));
      u32 ncap = 2 * eb->eb.cap;

      string *keys = lrealloc (alloc, eb->eb.keys, ncap * sizeof (string));
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }

      eb->eb.keys = keys;
      eb->eb.cap = ncap;
    }

  eb->eb.keys[eb->eb.len++] = t.str;
  eb->eb.state = EB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
eb_handle_comma_or_right (type_bldr *eb, token t)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);
  ASSERT (eb->eb.state == EB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      eb->eb.state = EB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }

  return TPR_SYNTAX_ERROR;
}

static inline tp_result
eb_accept_token (type_bldr *eb, token t, lalloc *alloc)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);

  switch (eb->eb.state)
    {
    case EB_WAITING_FOR_LB:
      {
        return eb_handle_waiting_for_lb (eb, t);
      }

    case EB_WAITING_FOR_IDENT:
      {
        return enbldr_push_str (eb, t, alloc);
      }

    case EB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return eb_handle_comma_or_right (eb, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
eb_build (type_bldr *eb, lalloc *alloc)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_STRUCT);
  ASSERT (eb->eb.state == EB_WAITING_FOR_COMMA_OR_RIGHT);

  string *strs = eb->eb.keys;

  eb->ret.en = lmalloc (alloc, sizeof *eb->ret.en);
  if (!eb->ret.en)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (eb->eb.len < eb->eb.cap)
    {
      strs = lrealloc (alloc, strs, eb->eb.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  eb->ret.en->keys = strs;
  eb->ret.en->len = eb->eb.len;
  eb->ret.type = T_UNION;

  return TPR_DONE;
}

//////////////// VARRAY BUILDER
static inline DEFINE_DBG_ASSERT_I (varray_bldr, varray_bldr, v)
{
  ASSERT (v);
  ASSERT (v->rank > 0);
}

tp_result
varray_bldr_create (varray_bldr *dest)
{
  ASSERT (dest);

  dest->rank = 1;
  dest->state = VAB_WAITING_FOR_LEFT_OR_TYPE;

  varray_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

/**
static inline tp_result
varray_bldr_incr (varray_bldr *sb)
{
  varray_bldr_assert (sb);
  ASSERT (sb->state == VAB_WAITING_FOR_RIGHT);
  sb->rank++;
  sb->state = VAB_WAITING_FOR_LEFT_OR_TYPE;
  return TPR_EXPECT_NEXT_TOKEN;
}
*/

//////////////// SARRAY BUILDER
DEFINE_DBG_ASSERT_I (sarray_bldr, sarray_bldr, s)
{
  ASSERT (s);
  ASSERT (s->dims);
  ASSERT (s->len > 0);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->len);

  for (u32 i = 0; i < s->len; ++i)
    {
      ASSERT (s->dims[i] > 0);
    }
}

tp_result
sarray_bldr_create (sarray_bldr *dest, u32 dim, lalloc *alloc)
{
  ASSERT (dest);

  dest->dims = lmalloc (alloc, 5 * sizeof *dest->dims);
  if (!dest->dims)
    {
      return TPR_MALLOC_ERROR;
    }
  dest->cap = 5;
  dest->len = 0;
  dest->dims[dest->len++] = dim;
  dest->state = SAB_WAITING_FOR_RIGHT;

  sarray_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

tp_result
sarray_bldr_incr (type_bldr *sb, token t, lalloc *alloc)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_SARRAY);
  ASSERT (sb->sab.state == SAB_WAITING_FOR_NUMBER);

  if (t.type != TT_INTEGER)
    {
      return TPR_SYNTAX_ERROR;
    }
  if (t.integer <= 0)
    {
      return TPR_SYNTAX_ERROR;
    }
  // TODO - maximum dimension size?

  // Check for size adjustments
  if (sb->sab.len == sb->sab.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (sb->sab.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sab.cap, sizeof (string)));
      u32 ncap = 2 * sb->sab.cap;

      u32 *dims = lrealloc (alloc, sb->sab.dims, ncap * sizeof (string));
      if (!dims)
        {
          return TPR_MALLOC_ERROR;
        }

      sb->sab.dims = dims;
      sb->sab.cap = ncap;
    }

  sb->sab.dims[sb->sab.len++] = (u32)t.integer;
  sb->sab.state = SAB_WAITING_FOR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

//////////////// PRIM BUILDER
static inline tp_result
prim_create (type_bldr *tb, prim_t p)
{
  type_bldr_assert (tb);
  ASSERT (tb->state == TB_UNKNOWN);

  tb->ret.p = p;
  tb->ret.type = T_PRIM;
  tb->state = TB_PRIM;

  return TPR_DONE;
}

//////////////// GENERIC BUILDER
static tp_result
tb_accept_token (type_bldr *tb, token t, lalloc *alloc)
{
  type_bldr_assert (tb);

  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_STRUCT:
            {
              return stbldr_create (tb, alloc);
            }
          case TT_UNION:
            {
              return unbldr_create (tb, alloc);
            }
          case TT_ENUM:
            {
              return enbldr_create (tb, alloc);
            }
          case TT_LEFT_BRACKET:
            {
              // TODO
              ASSERT (0);
              return 0;
            }
          case TT_PRIM:
            {
              return prim_create (tb, t.prim);
            }
          default:
            {
              return TPR_SYNTAX_ERROR;
            }
          }
      }
    case TB_ARRAY_UNKNOWN:
      {
        ASSERT (0);
        return 0;
      }
    case TB_STRUCT:
      {
        return sb_accept_token (tb, t, alloc);
      }
    case TB_UNION:
      {
        return ub_accept_token (tb, t, alloc);
      }
    case TB_ENUM:
      {
        return eb_accept_token (tb, t, alloc);
      }
    case TB_SARRAY:
      {
        ASSERT (0);
        return 0;
      }
    case TB_VARRAY:
      {
        ASSERT (0);
        return 0;
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
  ASSERT (0);
  return 0;
}

static tp_result
tb_accept_type (type_bldr *tb, type t)
{
  type_bldr_assert (tb);
  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return sb_accept_type (tb, t);
      }
    case TB_UNION:
      {
        return ub_accept_type (tb, t);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static tp_result
tb_build (type_bldr *tb, lalloc *alloc)
{
  type_bldr_assert (tb);

  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return sb_build (tb, alloc);
      }
    case TB_UNION:
      {
        return ub_build (tb, alloc);
      }
    case TB_ENUM:
      {
        return eb_build (tb, alloc);
      }
    case TB_PRIM:
      {
        return TPR_DONE;
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
tp_feed_token (type_parser *tp, token t)
{
  tp_result ret = tb_accept_token (
      &tp->stack[tp->sp - 1], t, tp->type_allocator);

  // We expect a new type
  // Push a type builder to
  // the top of the stack
  if (ret == TPR_EXPECT_NEXT_TYPE)
    {
      type_bldr next;
      next.state = TB_UNKNOWN;
      tp->stack[tp->sp++] = next;
      return TPR_EXPECT_NEXT_TOKEN;
    }

  // Otherwise, the acceptor is done
  // and can be reduced with the previous
  // builders
  //
  // sp == 1 means we're at the base
  while (tp->sp > 1 && ret == TPR_DONE)
    {
      // Pop the top off the stack
      type_bldr top = tp->stack[--tp->sp];
      if ((ret = tb_build (&top, tp->type_allocator)) != TPR_DONE)
        {
          return ret;
        }

      // Merge it with the previous
      ret = tb_accept_type (&tp->stack[tp->sp - 1], top.ret);
    }

  return ret;
}

void
type_parser_release (type_parser *t)
{
  type_parser_assert (t);
  lfree (t->stack_allocator, t->stack);
}
