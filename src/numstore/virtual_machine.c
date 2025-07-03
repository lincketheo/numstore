#include "numstore/virtual_machine.h"
#include "compiler/parser.h"
#include "core/ds/cbuffer.h"
#include "core/intf/stdlib.h"
#include "numstore/cursor/cursor.h"

struct vm_s
{
  cursor *c;

  cbuffer *input;
  cbuffer output;
  char _output[256];

  struct
  {
    const char *data;
    u16 pos;
    u16 len;
  };

  bool is_active;
  statement_result active;
};

static const char *good = "OK";
static const char *bad = "ERROR";

DEFINE_DBG_ASSERT_I (vm, vm, v)
{
  ASSERT (v);
  ASSERT (v->input);
  ASSERT (v->c);
}

static const char *TAG = "Virtual Machine";

vm *
vm_open (pager *p, cbuffer *input, error *e)
{
  vm *ret = i_malloc (1, sizeof *ret);
  ASSERT (input->cap % sizeof (query) == 0);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate virtual machine", TAG);
      return NULL;
    }

  cursor *c = cursor_open (p, e);
  if (c == NULL)
    {
      return NULL;
    }

  *ret = (vm){
    .c = c,
    .input = input,
    .output = cbuffer_create_from (ret->_output),

    .is_active = false,
    // active
  };

  vm_assert (ret);

  return ret;
}

void
vm_close (vm *v)
{
  vm_assert (v);
  cursor_close (v->c);
  i_free (v);
}

static inline void
create_query_execute (vm *v)
{
  vm_assert (v);
  ASSERT (v->is_active);
  ASSERT (v->active.stmt->q.type == QT_CREATE);

  if (v->pos == 0)
    {
      i_log_create (&v->active.stmt->q.create);
      error e = error_create (NULL);
      err_t ret = cursor_create (v->c, &v->active.stmt->q.create, &e);
      if (ret)
        {
          error_log_consume (&e);
          v->len = i_unsafe_strlen (bad) + sizeof v->len;
          v->data = bad;
        }
      else
        {
          v->len = i_unsafe_strlen (good) + sizeof v->len;
          v->data = good;
        }
    }

  // Write header
  if (v->pos < sizeof v->len)
    {
      ASSERT (v->output.cap >= sizeof (v->len)); // Assumes buffer can hold a u16
      u32 written = cbuffer_write (&v->len, sizeof v->len, 1, &v->output);
      ASSERT (written == 1);
      v->pos = sizeof (v->len);
    }

  // Write body
  if (v->pos >= sizeof v->len)
    {
      const char *head = v->data + (v->pos - sizeof v->len);

      u32 remaining = i_unsafe_strlen (v->data) - (v->pos - sizeof v->len);

      v->pos += cbuffer_write (head, 1, remaining, &v->output);
      ASSERT (v->pos <= i_unsafe_strlen (v->data) + sizeof v->len);

      if (v->pos == i_unsafe_strlen (v->data) + sizeof v->len)
        {
          v->pos = 0;
          v->is_active = false;
          return;
        }
    }
}

static inline void
delete_query_execute (vm *v)
{
  vm_assert (v);
  ASSERT (v->is_active);
  ASSERT (v->active.stmt->q.type == QT_DELETE);

  if (v->pos == 0)
    {
      i_log_delete (&v->active.stmt->q.delete);
      error e = error_create (NULL);
      err_t ret = cursor_delete (v->c, &v->active.stmt->q.delete, &e);
      if (ret)
        {
          error_log_consume (&e);
          v->len = i_unsafe_strlen (bad) + sizeof v->len;
          v->data = bad;
        }
      else
        {
          v->len = i_unsafe_strlen (good) + sizeof v->len;
          v->data = good;
        }
    }

  // Write header
  if (v->pos < sizeof v->len)
    {
      ASSERT (v->output.cap >= sizeof (v->len)); // Assumes buffer can hold a u16
      u32 written = cbuffer_write (&v->len, sizeof v->len, 1, &v->output);
      ASSERT (written == 1);
      v->pos = sizeof (v->len);
    }

  // Write body
  if (v->pos >= sizeof v->len)
    {
      const char *head = v->data + (v->pos - sizeof v->len);

      u32 remaining = i_unsafe_strlen (v->data) - (v->pos - sizeof v->len);

      v->pos += cbuffer_write (head, 1, remaining, &v->output);
      ASSERT (v->pos <= i_unsafe_strlen (v->data) + sizeof v->len);

      if (v->pos == i_unsafe_strlen (v->data) + sizeof v->len)
        {
          v->pos = 0;
          v->is_active = false;
          return;
        }
    }
}

static inline void
insert_query_execute (vm *v)
{
  vm_assert (v);
  ASSERT (v->is_active);
  ASSERT (v->active.stmt->q.type == QT_INSERT);

  if (v->pos == 0)
    {
      i_log_insert (&v->active.stmt->q.insert);
      error e = error_create (NULL);
      err_t ret = cursor_insert (v->c, &v->active.stmt->q.insert, &e);
      if (ret)
        {
          error_log_consume (&e);
          v->len = i_unsafe_strlen (bad) + sizeof v->len;
          v->data = bad;
        }
      else
        {
          v->len = i_unsafe_strlen (good) + sizeof v->len;
          v->data = good;
        }
    }

  // Write header
  if (v->pos < sizeof v->len)
    {
      ASSERT (v->output.cap >= sizeof (v->len)); // Assumes buffer can hold a u16
      u32 written = cbuffer_write (&v->len, sizeof v->len, 1, &v->output);
      ASSERT (written == 1);
      v->pos = sizeof (v->len);
    }

  // Write body
  if (v->pos >= sizeof v->len)
    {
      const char *head = v->data + (v->pos - sizeof v->len);

      u32 remaining = i_unsafe_strlen (v->data) - (v->pos - sizeof v->len);

      v->pos += cbuffer_write (head, 1, remaining, &v->output);
      ASSERT (v->pos <= i_unsafe_strlen (v->data) + sizeof v->len);

      if (v->pos == i_unsafe_strlen (v->data) + sizeof v->len)
        {
          v->pos = 0;
          v->is_active = false;
          return;
        }
    }
}

static inline void
error_query_execute (vm *v)
{
  vm_assert (v);
  ASSERT (v->is_active);

  // Write header
  if (v->pos < sizeof v->len)
    {
      ASSERT (v->output.cap >= sizeof (v->len)); // Assumes buffer can hold a u16
      u32 written = cbuffer_write (&v->len, sizeof v->len, 1, &v->output);
      v->pos += written * sizeof v->len;
      ASSERT (v->pos <= sizeof (v->len));
    }

  // Write body
  if (v->pos >= sizeof v->len)
    {
      char *head = v->active.e.cause_msg + (v->pos - sizeof v->len);
      u32 remaining = v->active.e.cmlen - (v->pos - sizeof v->len);
      v->pos += cbuffer_write (head, 1, remaining, &v->output);
      ASSERT (v->pos <= v->active.e.cmlen + sizeof v->len);

      if (v->pos == v->active.e.cmlen + sizeof v->len)
        {
          v->pos = 0;
          v->is_active = false;
          return;
        }
    }
  return;
}

cbuffer *
vm_get_output (vm *v)
{
  vm_assert (v);
  return &v->output;
}

void
vm_execute_one_query (vm *v)
{
  vm_assert (v);
  ASSERT (v->is_active);

  if (!v->active.ok)
    {
      error_query_execute (v);
      return;
    }

  switch (v->active.stmt->q.type)
    {
    case QT_CREATE:
      {
        create_query_execute (v);
        break;
      }
    case QT_DELETE:
      {
        delete_query_execute (v);
        break;
      }
    case QT_INSERT:
      {
        insert_query_execute (v);
        break;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

void
vm_execute (vm *v)
{
  vm_assert (v);

  while (true)
    {
      // Execute active query
      if (v->is_active)
        {
          vm_execute_one_query (v);
          v->is_active = false;
        }

      // Block on downstream
      if (cbuffer_avail (&v->output) < 1)
        {
          return;
        }

      // Block on upstream
      if (cbuffer_len (v->input) < sizeof (query))
        {
          return;
        }

      // Read the next query
      if (!v->is_active)
        {
          cbuffer_read_expect (&v->active, sizeof (v->active), 1, v->input);
          v->is_active = true;
        }
    }
}
