#pragma once

/* Memory region attributes encoding */

#define MEMATTR_RA  1
#define MEMATTR_NRA 0

#define MEMATTR_WA  1
#define MEMATTR_NWA 0

#define MEMATTR_DEVICE_NGNRNE 0b00000000
#define MEMATTR_DEVICE_NGNRE  0b00000100
#define MEMATTR_DEVICE_NGRE   0b00001000
#define MEMATTR_DEVICE_GRE    0b00001100

#define MEMATTR_NON_CACHEABLE()             0b0100
#define MEMATTR_WRITETHROUGH_TRANS(R, W)    (0b0000 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITEBACK_TRANS(R, W)       (0b0100 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITETHROUGH_NONTRANS(R, W) (0b1000 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITEBACK_NONTRANS(R, W)    (0b1100 | ((R & 1) << 1) | (W & 1))
#define MAKE_MEMATTR_NORMAL(oooo, iiii)     (((oooo & 0xf) << 4) | (iiii & 0xf))


