#pragma once

#define optimized __attribute__((optimize("O3")))
#define packed    __attribute__((packed))
#define noret     __attribute__((noreturn))
#define section(name) __attribute__((section(name)))
#define weak_symbol __attribute__((weak))

