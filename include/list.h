#pragma once
#include <refs.h>
#include <barriers.h>

struct list_head {
  struct list_head *next;
  struct list_head *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
  struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
  list->next = list;
	list->prev = list;
}

static inline int list_empty(const struct list_head *head)
{
  DATA_BARRIER;
  return head->next == head;
}

static inline void __list_add(struct list_head *new, 
  struct list_head *prev,
  struct list_head *next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  DATA_BARRIER;
  prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
  __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
  __list_add(new, head->prev, head);
}

#define list_entry(ptr, type, member) \
  container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
  list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_next_entry_or_null(ptr, type, member) ({ \
    struct list_head *head__ = (ptr); \
    struct list_head *pos__ = head__->next; \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
    })

#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member) \
  for (pos = list_first_entry(head, typeof(*pos), member); \
      &pos->member != (head); \
      pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, tmp, head, member) \
  for (pos = list_first_entry(head, typeof(*pos), member),\
      tmp = list_next_entry(pos, member);\
      &pos->member != (head);\
      pos = tmp, tmp = list_next_entry(tmp, member))


static inline void __list_del(struct list_head *entry)
{
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
  entry->next = NULL;
  entry->prev = NULL;
}

static inline void list_del(struct list_head *entry)
{
  __list_del(entry);
}

static inline void list_del_init(struct list_head *entry)
{
  __list_del(entry);
  entry->prev = entry;
  entry->next = entry;
}

static inline void list_move(struct list_head *list, 
    struct list_head *head)
{
  __list_del(list);
  list_add(list, head);
}
