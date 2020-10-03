#pragma once
#include <types.h>

void mmu_init(void);

void mmu_enable_configured(void);

/*
 * Print tranlation information for virtual address.
 * va - virtual address to get information for.
 * verbose - if 0 provide a one line description
 *         - if 1 print more info in more lines
 */
void mmu_print_va(uint64_t va, int verbose);
