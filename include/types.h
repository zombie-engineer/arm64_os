#pragma once

#ifndef TEST_STRING
typedef char                 int8_t;
typedef unsigned char       uint8_t;

typedef          short      int16_t;
typedef unsigned short     uint16_t;

typedef          int        int32_t;
typedef unsigned int       uint32_t;

typedef          long long  int64_t;
typedef unsigned long long uint64_t;
typedef          int           bool;
#define true 1
#define false 0

typedef __SIZE_TYPE__  size_t;
typedef size_t ssize_t;
#else
#include <stdint.h>
#include <stddef.h>
#endif
