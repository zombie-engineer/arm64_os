#pragma once
#include <types.h>

void enable_irq(void);
void disable_irq(void);
int is_irq_enabled(void);

int mem_model_max_pa_bits(void);
int mem_model_num_asid_bits(void);
int mem_model_4k_granule_support(void);
int mem_model_16k_granule_support(void);
int mem_model_64k_granule_support(void);
int mem_model_st2_4k_granule_support(void);
int mem_model_st2_16k_granule_support(void);
int mem_model_st2_64k_granule_support(void);
int mem_cache_get_il1_line_size(void);

void disable_l1_caches(void);

#define CACHE_TYPE_D 0
#define CACHE_TYPE_I 1
#define CACHE_TYPE_MAX CACHE_TYPE_I

#define CACHE_L1     0
#define CACHE_L2     1
#define CACHE_LEVEL_MAX CACHE_L2

int mem_cache_get_line_size(int cache_level, int cache_type);
int mem_cache_get_num_sets(int cache_level, int cache_type);
int mem_cache_get_num_ways(int cache_level, int cache_type);

unsigned long arm_get_ttbr0_el1();
unsigned long arm_get_ttbr1_el1();
unsigned long arm_get_tcr_el1();

void arm_set_ttbr0_el1(unsigned long);
void arm_set_ttbr1_el1(unsigned long);
void arm_set_tcr_el1(unsigned long );

void arm_mmu_init(void);
// void armv8_enable_mmu(long long ttbr0, long long ttbr1);

void armv8_set_mair_el1(uint64_t);

uint64_t armv8_get_mair_el1();

// Returns system clock frequency in Hz
uint32_t get_system_timer_freq(void);

// Returns system clock counter value
uint64_t get_system_timer(void);

uint64_t armv8_get_mpidr_el1(int core_num);
