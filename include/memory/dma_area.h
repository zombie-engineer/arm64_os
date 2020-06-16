#pragma once
#include <types.h>

void *dma_alloc(int sz);

void dma_free(void *);

void dma_area_init(void);

uint64_t dma_area_get_start_addr(void);

uint64_t dma_area_get_end_addr(void);
