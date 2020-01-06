#pragma once
#include <arch/armv8/cpu_context.h>

extern aarch64_cpuctx_t __armv8_cpuctx_e;

extern void __armv8_set_elr_el1(uint64_t value);

extern void __armv8_set_sp_el1(uint64_t value);

extern noret void __armv8_cpuctx_eret(void);
