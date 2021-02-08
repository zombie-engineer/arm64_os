#include <semaphore.h>
#include <sched.h>

struct semaphore_waiter {
  struct list_head list;
  struct task *waiter_task;
  atomic_t waitflag;
};

static inline void __semaphore_down(struct semaphore *s)
{
  struct semaphore_waiter waiter;

  list_add_tail(&waiter.list, &s->wait_list);
  waiter.waiter_task = get_current();
  waitflag_init(&waiter.waitflag);

  while(1) {
    spinlock_unlock_irq(&s->lock);
    wait_on_waitflag(&waiter.waitflag);
    spinlock_lock_irq(&s->lock);
    if (waiter.waitflag)
      break;
  }
}

static inline void __semaphore_up(struct semaphore *s)
{
  struct semaphore_waiter *waiter;

  waiter = list_first_entry(&s->wait_list, struct semaphore_waiter, list);
  wakeup_waitflag(&waiter->waitflag);
}

void semaphore_down(struct semaphore *s)
{
  int flag;
  spinlock_lock_disable_irq(&s->lock, flag);
  if (s->count > 0)
    s->count--;
  else
    __semaphore_down(s);
  spinlock_unlock_restore_irq(&s->lock, flag);
}

void semaphore_up(struct semaphore *s)
{
  int flag;
  spinlock_lock_disable_irq(&s->lock, flag);
  if (list_empty(&s->wait_list))
    s->count++;
  else
    __semaphore_up(s);
  spinlock_unlock_restore_irq(&s->lock, flag);
}

int down_trylock(struct semaphore *s)
{
  int flags;
  uint64_t count;

  spinlock_lock_disable_irq(&s->lock, flags);
  count = s->count - 1;
  if (count >= 0)
    s->count = count;
  spinlock_unlock_restore_irq(&s->lock, flags);
  return count < 0;
}
