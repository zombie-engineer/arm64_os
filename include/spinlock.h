#pragma once

#define mutex_lock __armv8_mutex_lock
#define mutex_unlock __armv8_mutex_unlock
extern void __armv8_mutex_lock(void *mutex);
extern void __armv8_mutex_unlock(void *mutex);
