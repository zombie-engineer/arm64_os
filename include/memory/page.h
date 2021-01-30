#pragma once
#include <list.h>

struct page {
  void *addr;
  struct list_head list;
};
