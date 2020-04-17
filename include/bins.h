#pragma once

#define BIN_START(x) _binary_bins_ ## x ## _bin_start
#define BIN_END(x) _binary_bins_ ## x ## _bin_end
#define BIN_SIZE(x) &BIN_END(x) - &BIN_START(x)

#define DECL_BIN_OBJECTS(x) \
  extern char BIN_START(x);\
  extern char BIN_END(x);\
  static inline void *bins_get_start_ ## x() \
  { \
    return &BIN_START(x);\
  } \
  static inline int bins_get_size_ ## x()\
  { \
    return BIN_SIZE(x);\
  }

DECL_BIN_OBJECTS(atmega)
DECL_BIN_OBJECTS(nokia5110_animation)
DECL_BIN_OBJECTS(font)
