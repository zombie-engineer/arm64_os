#pragma once

#define spinlock_init __armv8_spinlock_init
#define spinlock_lock __armv8_spinlock_lock
#define spinlock_unlock __armv8_spinlock_unlock

extern void __armv8_spinlock_init(void *spinlock);
extern void __armv8_spinlock_lock(void *spinlock);
extern void __armv8_spinlock_unlock(void *spinlock);
