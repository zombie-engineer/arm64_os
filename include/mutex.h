#pragma once
#include <atomic.h>
#include <spinlock.h>
#include <list.h>

struct mutex {
  atomic_t owner;
  struct spinlock wait_lock;
  struct list_head wait_list;
};

static inline void mutex_init(struct mutex *lock)
{
  lock->owner = 0;
  __spinlock_init(&lock->wait_lock);
  INIT_LIST_HEAD(&lock->wait_list);
}

static inline bool mutex_is_locked(struct mutex *lock)
{
  return lock->owner != 0;
}

void mutex_lock(struct mutex *lock);
int mutex_trylock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);

static inline int mutex_lock_interruptible(struct mutex *lock)
{
  mutex_lock(lock);
  return 1;
}
