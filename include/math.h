#pragma once

static inline int get_biggest_log2(int num)
{
  int res = 0;
  int tmp;

  tmp = num - 1;
  while(tmp > 1) {
    res++;
    tmp >>= 1;
  }
  if ((tmp << res) < num)
    res++;
  return res;
}

