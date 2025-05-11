#include "type/builders/enum.h"
#include "dev/assert.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "type/types.h"
#include "type/types/enum.h"

DEFINE_DBG_ASSERT_I (enum_builder, enum_builder, s)
{
  ASSERT (s);

  ASSERT (s->adcap > 0);
  ASSERT (s->adlen <= s->adcap);

  ASSERT (s->ascap > 0);
  ASSERT (s->aslen <= s->ascap);

  ASSERT (s->alldata);
  ASSERT (s->adlen > 0);
}

static inline err_t
enb_type_error (const char *msg, error *e)
{
  return error_causef (
      e, ERR_INVALID_TYPE,
      "Enum Builder: "
      "Invalid value encountered. %s",
      msg);
}

static inline err_t
enb_alloc_err (const char *thing, error *e)
{
  return error_causef (
      e, ERR_NOMEM,
      "Enum Builder: "
      "Failed to allocate memory for %s",
      thing);
}

err_t
enb_create (enum_builder *dest, lalloc *alloc, error *e)
{
  ASSERT (dest);

  lalloc_r alldata = lmalloc (alloc, 10, 2, sizeof *dest->alldata);
  if (alldata.stat != AR_SUCCESS)
    {
      return enb_alloc_err ("enum keys buffer", e);
    }

  lalloc_r allstrs = lmalloc (alloc, 10, 10, sizeof *dest->allstrs);
  if (allstrs.stat != AR_SUCCESS)
    {
      return enb_alloc_err ("keys data", e);
    }

  *dest = (enum_builder){
    .alldata = alldata.ret,
    .adlen = 0,
    .adcap = alldata.rlen,

    .allstrs = allstrs.ret,
    .aslen = 0,
    .ascap = allstrs.rlen,

    .alloc = alloc,
  };

  enum_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
enb_resize_alldata (enum_builder *eb, u32 req, u32 min, error *e)
{
  enum_builder_assert (eb);

  if (eb->adcap == req)
    {
      return SUCCESS;
    }

  lalloc_r alldata = lrealloc (
      eb->alloc,
      eb->alldata,
      req, min, 1);

  if (alldata.stat != AR_SUCCESS)
    {
      return enb_alloc_err ("key data", e);
    }

  eb->adcap = alldata.rlen;
  eb->alldata = alldata.ret;

  return SUCCESS;
}

static inline err_t
enb_resize_allstrs (enum_builder *eb, u32 req, u32 min, error *e)
{
  enum_builder_assert (eb);

  if (eb->ascap == req)
    {
      return SUCCESS;
    }

  lalloc_r allstrs = lrealloc (
      eb->alloc,
      eb->alldata,
      req, min,
      sizeof *eb->allstrs);

  if (allstrs.stat != AR_SUCCESS)
    {
      return enb_alloc_err ("key data", e);
    }

  eb->ascap = allstrs.rlen;
  eb->allstrs = allstrs.ret;

  return SUCCESS;
}

static inline string
enb_get_key (const enum_builder *eb, u32 idx)
{
  enum_builder_assert (eb);
  ASSERT (idx < eb->aslen);
  return (string){
    .data = &eb->alldata[eb->allstrs[idx].idx],
    .len = eb->allstrs[idx].klen,
  };
}

static inline bool
enb_has_key_been_used (const enum_builder *eb, const string key)
{
  for (u32 i = 0; i < eb->aslen; ++i)
    {
      if (string_equal (enb_get_key (eb, i), key))
        {
          return true;
        }
    }
  return false;
}

err_t
enb_accept_key (enum_builder *eb, const string key, error *e)
{
  enum_builder_assert (eb);

  if (enb_has_key_been_used (eb, key))
    {
      return enb_type_error ("Duplicate type", e);
    }

  /**
   * Expand buffers if needed
   */
  if (eb->adlen + key.len > eb->adcap)
    {
      err_t_wrap (enb_resize_alldata (eb, eb->adcap * 2, eb->adcap + 1, e), e);
    }
  if (eb->aslen == eb->ascap)
    {
      err_t_wrap (enb_resize_allstrs (
                      eb,
                      eb->ascap + key.len * 3,
                      eb->ascap + key.len, e),
                  e);
    }

  // Append string
  eb->allstrs[eb->aslen].klen = key.len;
  eb->allstrs[eb->aslen++].idx = eb->adlen;

  // Append raw data
  i_memcpy (&eb->alldata[eb->adlen], key.data, key.len);
  eb->adlen += key.len;

  return SUCCESS;
}

static inline err_t
enb_move_strings (enum_t *dest, enum_builder *eb, error *e)
{
  enum_builder_assert (eb);
  ASSERT (dest);

  lalloc_r strings = lmalloc (
      eb->alloc,
      eb->aslen,
      eb->aslen,
      sizeof (string));

  if (strings.stat != AR_SUCCESS)
    {
      return enb_alloc_err ("strings", e);
    }

  string *keys = strings.ret;

  for (u32 i = 0; i < eb->aslen; ++i)
    {
      keys[i] = enb_get_key (eb, i);
    }

  dest->keys = keys;
  dest->len = eb->aslen;

  lfree (eb->alloc, eb->allstrs);
  eb->allstrs = NULL;
  eb->aslen = 0;
  eb->ascap = 0;

  return SUCCESS;
}

err_t
enb_build (enum_t *dest, enum_builder *eb, error *e)
{
  enum_builder_assert (eb);
  ASSERT (dest);

  if (eb->aslen == 0)
    {
      return enb_type_error ("Expecting at least 1 key", e);
    }

  enum_t ret;

  /**
   * Expand alldata to fit the keys at the end
   */
  err_t_wrap (enb_resize_alldata (eb, eb->adlen, eb->adlen, e), e);
  ret.data = eb->alldata;

  /**
   * Convert enum builder strings to real strings
   */
  err_t_wrap (enb_move_strings (&ret, eb, e), e);

  *dest = ret;

  return SUCCESS;
}

void
enb_free (enum_builder *enb)
{
  enum_builder_assert (enb);
  lfree (enb->alloc, enb->alldata);
  lfree (enb->alloc, enb->allstrs);
}

TEST (enum_builder_green_path)
{
  lalloc alloc = lalloc_create (1000);
  enum_builder enb;
  error e = error_create (NULL);

  test_fail_if (enb_create (&enb, &alloc, &e));

  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("foo"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("bar"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("biz"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("buz"), &e));

  enum_t en;
  test_fail_if (enb_build (&en, &enb, &e));

  test_assert_int_equal (en.len, 4);
  test_assert_int_equal (string_equal (en.keys[0], unsafe_cstrfrom ("foo")), true);
  test_assert_int_equal (string_equal (en.keys[1], unsafe_cstrfrom ("bar")), true);
  test_assert_int_equal (string_equal (en.keys[2], unsafe_cstrfrom ("biz")), true);
  test_assert_int_equal (string_equal (en.keys[3], unsafe_cstrfrom ("buz")), true);

  enum_t_free_internals (&en, &alloc);
  test_assert_int_equal (alloc.used, 0);
}

TEST (enum_builder_red_path_duplicate_strings)
{
  lalloc alloc = lalloc_create (1000);
  enum_builder enb;
  error e = error_create (NULL);

  test_fail_if (enb_create (&enb, &alloc, &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("foo"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("bar"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("biz"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("buz"), &e));
  test_fail_if (enb_accept_key (&enb, unsafe_cstrfrom ("buz"), &e));

  enum_t en;
  test_assert_int_equal (enb_build (&en, &enb, &e), ERR_INVALID_TYPE);

  enb_free (&enb);
  test_assert_int_equal (alloc.used, 0);
}

TEST (enum_builder_red_path_empty)
{
  lalloc alloc = lalloc_create (1000);
  enum_builder enb;
  error e = error_create (NULL);

  test_fail_if (enb_create (&enb, &alloc, &e));

  enum_t en;
  test_assert_int_equal (enb_build (&en, &enb, &e), ERR_INVALID_TYPE);

  enb_free (&enb);
  test_assert_int_equal (alloc.used, 0);
}
