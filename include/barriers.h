#pragma once

#define SYNC_BARRIER  asm volatile("dsb sy" ::: "memory")
#define DATA_BARRIER  asm volatile("dmb sy" ::: "memory")

