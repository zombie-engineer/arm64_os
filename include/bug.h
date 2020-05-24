#pragma once
#include <kernel_panic.h>

#define BUG(__expr, __string) if (__expr) kernel_panic(__string)
