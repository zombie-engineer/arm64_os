#pragma once
#include <spinlock.h>
#include <list.h>

struct semaphore {
  struct spinlock lock;
  uint32_t count;
  struct list_head wait_list;
};

static inline void sema_init(struct semaphore *sem, int val)
{
  sem->lock.lock = 0;
  sem->count = val;
  INIT_LIST_HEAD(&sem->wait_list);
}

void down(struct semaphore *sem);
int down_trylock(struct semaphore *sem);
void up(struct semaphore *sem);
