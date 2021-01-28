#include <semaphore.h>

void down(struct semaphore *sem)
{
}

void up(struct semaphore *sem)
{
}

int down_trylock(struct semaphore *sem)
{
  int flags;
  uint64_t count;

  spinlock_lock_disable_irq(&sem->lock, flags);
  count = sem->count - 1;
  if (count >= 0)
    sem->count = count;
  spinlock_unlock_restore_irq(&sem->lock, flags);
  return count < 0;
}
