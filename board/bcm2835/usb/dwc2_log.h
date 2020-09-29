#pragma once
#include <common.h>

extern int dwc2_log_level;

#define DWCINFO(fmt, ...) printf("[DWC2 INFO] "fmt __endline, ##__VA_ARGS__)
#define DWCDEBUG(fmt, ...) if (dwc2_log_level > 0)\
                          printf("[DWC2 DBG] %s: "fmt __endline, __func__, ##__VA_ARGS__)
#define DWCDEBUG2(fmt, ...) if (dwc2_log_level > 1)\
                          printf("[DWC2 DBG2] %s: "fmt __endline, __func__, ##__VA_ARGS__)
#define DWCERR(fmt, ...)  printf("[DWC2 ERR] %s: "fmt __endline, __func__, ##__VA_ARGS__)
#define DWCWARN(fmt, ...) printf("[DWC2 WARN] %s: "fmt __endline, __func__, ##__VA_ARGS__)

