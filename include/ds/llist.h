#pragma once

#include "dev/assert.h"
#include "intf/types.h"

#include <stddef.h> // offsetof

typedef struct llnode_s
{
  struct llnode_s *next;
} llnode;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr)-offsetof (type, member)))

static inline void
llnode_init (llnode *n)
{
  n->next = NULL;
}

static inline bool
list_is_empty (const llnode *head)
{
  return head == NULL;
}

static inline u32
list_length (const llnode *head)
{
  u32 len = 0;
  for (const llnode *cur = head; cur; cur = cur->next)
    {
      len++;
    }
  return len;
}

static inline void
list_push (llnode **head, llnode *n)
{
  n->next = *head;
  *head = n;
}

static inline void
list_append (llnode **head, llnode *n)
{
  n->next = NULL;
  if (!*head)
    {
      *head = n;
    }
  else
    {
      llnode *cur = *head;
      while (cur->next)
        {
          cur = cur->next;
        }
      cur->next = n;
    }
}

static inline llnode *
list_pop (llnode **head)
{
  if (!*head)
    return NULL;
  llnode *n = *head;
  *head = n->next;
  n->next = NULL;
  return n;
}

static inline llnode *
list_tail (llnode *head)
{
  if (!head)
    return NULL;
  while (head->next)
    {
      head = head->next;
    }
  return head;
}

static inline void
list_reverse (llnode **head)
{
  llnode *prev = NULL;
  llnode *cur = *head;
  while (cur)
    {
      llnode *next = cur->next;
      cur->next = prev;
      prev = cur;
      cur = next;
    }
  *head = prev;
}

static inline void
list_remove (llnode **head, llnode *n)
{
  llnode **cur = head;
  while (*cur && *cur != n)
    {
      cur = &(*cur)->next;
    }
  if (*cur)
    {
      *cur = n->next;
      n->next = NULL;
    }
}

static inline llnode *
llnode_get_n (llnode *head, u32 index)
{
  llnode *cur = head;
  for (u32 i = 0; cur && i < index; ++i)
    {
      cur = cur->next;
    }
  return cur;
}

// Iterate over list
#define LLIST_FOR_EACH(head, iter) \
  for (llnode *iter = (head); iter; iter = iter->next)

#pragma GCC diagnostic pop
