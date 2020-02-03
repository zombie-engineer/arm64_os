#pragma once
#include <types.h>

/*
 * Simplest checksum algorithm that adds 'size' bytes starting 
 * from 'buf' address adding 'initval' to result.
 */
uint32_t checksum_basic(const char *buf, int size, uint32_t initval);

