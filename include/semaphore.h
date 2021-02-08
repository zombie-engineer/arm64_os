#pragma once
#include <spinlock.h>
#include <list.h>

struct semaphore {
  struct spinlock lock;
  uint32_t count;
  struct list_head wait_list;
};

static inline void semaphore_init(struct semaphore *sem, int val)
{
  sem->lock.lock = 0;
  sem->count = val;
  INIT_LIST_HEAD(&sem->wait_list);
}

void semaphore_down(struct semaphore *sem);
int semaphore_down_trylock(struct semaphore *sem);
void semaphore_up(struct semaphore *sem);
