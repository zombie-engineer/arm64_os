#pragma once

/*
 * D13.2.84 MAIR_EL1, Memory Attribute Indirection Register (EL1)
 * Provides the memory attribute encodings corresponding to the possible AttrIndx values in a
 * Long-descriptor format translation table entry for stage 1 translations at EL1.
 * Attr<n>, bits [8n+7:8n], for n = 0 to 7
 *    0b0000dd00 Device memory. 
 *    0b0000ddxx (xx != 00) unpredictable
 *    0booooiiii (oooo != 0000 and iiii != 0000) Normal memory 
 *    0b11110000 Tagged Normal Memory.
 *    0bxxxx0000 (xxxx != 0000 and xxxx != 1111) unpredictable
 *    dd: 0b00 Device-nGnRnE memory
 *        0b01 Device-nGnRE memory
 *        0b10 Device-nGRE memory
 *        0b11 Device-GRE memory
 *    oooo: 0b0000
 *          0b00RW, RW not 0b00 Normal mem, Outer Write-throught Transient
 *          0b0100              Normal mem, Outer Non-cacheable
 *          0b01RW, RW not 0b00 Normal mem, Outer Write-Back Transient
 *          0b11RW              Normal mem, Outer Write-Back Non-transient
 *    iiii: 0b0000
 */
/* Memory region attributes encoding */

#define MEMATTR_RA  1
#define MEMATTR_NRA 0

#define MEMATTR_WA  1
#define MEMATTR_NWA 0

/* Device memory.
 * Memory attributes for device memory consist of 3 params:
 * G or nG - Gathering or Non-Gathering
 * R or nR - Reordering or Non-Reordering
 * E or nE - Execution of No Execution bit 
 * Gathering means if multiple accesses to memory can be combined in
 * one big access.
 *
 * Reordering means if the data bus controller can reorder reads or writes
 * to memory in optimized order
 *
 * Execution bit means if confirmation of a write operation to memory must
 * be given when the data has actually landed the target address or not
 */
#define MEMATTR_DEVICE_NGNRNE 0b00000000
#define MEMATTR_DEVICE_NGNRE  0b00000100
#define MEMATTR_DEVICE_NGRE   0b00001000
#define MEMATTR_DEVICE_GRE    0b00001100

/* Normal memory
 * Consists of 2 packs of parameters - for Inner and Outer shareability
 * For both inner and outer shareable types the cache might be
 *  - Write-Through - data written to memory is written both to cache and to target memory
 *  - Write-Back    - data is only written to cache and then when evicted written to memory
 *  - Non-Cacheable at all 
 *  - Transient / NonTransient - a hint for cache system, wheather to put the data into 
 *    cache or not
 * All cacheable types also have Read Allocate / Write allocate possible policies
 * Read Allocate - data will be put to cache to read.
 * Write Allocate - data will be put to cache at write or at read.
 *
 * Write-Back Write Allocate Transient Normal Cache
 * Would mean that at write operation the target memory location would be 
 * first put to cache (Write Allocate), then written in cache and marked as dirty.
 * Later when the cache should be eviceted, the dirty flag would tell the system
 * to write cache contents to memory. 
 */
#define MEMATTR_NON_CACHEABLE()             0b0100
#define MEMATTR_WRITETHROUGH_TRANS(R, W)    (0b0000 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITEBACK_TRANS(R, W)       (0b0100 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITETHROUGH_NONTRANS(R, W) (0b1000 | ((R & 1) << 1) | (W & 1))
#define MEMATTR_WRITEBACK_NONTRANS(R, W)    (0b1100 | ((R & 1) << 1) | (W & 1))

#define MAKE_MEMATTR_NORMAL(oooo, iiii)     (((oooo & 0xf) << 4) | (iiii & 0xf))

typedef struct mair_repr_64 {
  char memattrs[8];
} mair_repr_64_t;

#define __mair_put(r, i) ((uint64_t)(r)->memattrs[i] << (8 * i))

#define mair_repr_64_to_value(r) \
  (uint64_t)(\
      __mair_put(r, 0)|\
      __mair_put(r, 1)|\
      __mair_put(r, 2)|\
      __mair_put(r, 3)|\
      __mair_put(r, 4)|\
      __mair_put(r, 5)|\
      __mair_put(r, 6)|\
      __mair_put(r, 7)\
      )
// #undef mair_put

