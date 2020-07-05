#pragma once
#include <types.h>
#include <compiler.h>
#include <cpu.h>

#define DECL_SPINLOCK(name) ALIGNED(64) uint64_t name
#define DECL_COND_SPINLOCK(name) ALIGNED(64) uint64_t name

#define __spinlock_init __armv8_spinlock_init
#define __spinlock_lock __armv8_spinlock_lock
#define __spinlock_unlock __armv8_spinlock_unlock

#define spinlock_init __spinlock_init
#define spinlock_lock __spinlock_lock
#define spinlock_unlock __spinlock_unlock

/*
 * conditional spinlock. Used in structures that should work
 * under both environments with spinlocks enabled and not.
 *
 * cond_spinlock_init               - initialize conditional spinlock
 * cond_spinlock_lock_irq           - lock if spinlock enabled in irq context
 * cond_spinlock_unlock_irq         - unlock is spinlock enabled in irq context
 * cond_spinlock_lock_disable_irq   - lock if spinlock enabled and ensure irq disabled,
 *                                    also save irq flags to restore to them later
 * cond_spinlock_unlock_restore_irq - unlock if spinlock enabled and restore irq flags
 */

#define cond_spinlock_init(__cond_spinlock)\
  __spinlock_init(__cond_spinlock)

#define cond_spinlock_lock_irq(__cond_spinlock)\
  do {\
    if (spinlocks_enabled) {\
      __spinlock_lock(__cond_spinlock);\
    }\
  } while(0)

#define cond_spinlock_unlock_irq(__cond_spinlock)\
  __spinlock_unlock(__cond_spinlock);\

#define cond_spinlock_lock_irq(__cond_spinlock)\
  do {\
    if (spinlocks_enabled) {\
      __spinlock_lock(__cond_spinlock);\
    }\
  } while(0)

#define cond_spinlock_lock_disable_irq(__cond_spinlock, __irqflags)\
  do {\
    disable_irq_save_flags(__irqflags);\
    cond_spinlock_lock_irq(__cond_spinlock);\
  } while(0)

#define cond_spinlock_unlock_restore_irq(__cond_spinlock, __irqflags)\
  do {\
    cond_spinlock_unlock_irq(__cond_spinlock);\
    restore_irq_flags(__irqflags);\
  } while(0)

extern uint64_t spinlocks_enabled;
extern void __armv8_spinlock_init(void *spinlock);
extern void __armv8_spinlock_lock(void *spinlock);
extern void __armv8_spinlock_unlock(void *spinlock);
