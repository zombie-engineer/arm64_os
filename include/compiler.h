#pragma once

#define OPTIMIZED      __attribute__((optimize("O3")))
#define PACKED         __attribute__((packed))
#define UNUSED         __attribute__((unused))
#define NORET          __attribute__((noreturn))
#define SECTION(name)  __attribute__((section(name)))
#define WEAK_SYMBOL    __attribute__((weak))
#define ALIGNED(bytes) __attribute__((aligned(bytes)))
#define NOINLINE       __attribute__((noinline))
