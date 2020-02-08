#pragma once
#include <types.h>
#include <compiler.h>

#define DECL_SPINLOCK(name) ALIGNED(64) uint64_t name
#define spinlock_init __armv8_spinlock_init
#define spinlock_lock __armv8_spinlock_lock
#define spinlock_unlock __armv8_spinlock_unlock

extern uint64_t spinlocks_enabled;
extern void __armv8_spinlock_init(void *spinlock);
extern void __armv8_spinlock_lock(void *spinlock);
extern void __armv8_spinlock_unlock(void *spinlock);
