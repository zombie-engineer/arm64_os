#pragma once
#include <types.h>

typedef void(*completion_fn)(void *);

struct completion {
  uint32_t done;
};
