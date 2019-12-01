#pragma once

/* Memory region attributes encoding */

#define MEMATTR_DEVICE_NGNRNE 0b00000000
#define MEMATTR_DEVICE_NGNRE  0b00000100
#define MEMATTR_DEVICE_NGRE   0b00001000
#define MEMATTR_DEVICE_GRE    0b00001100

#define MAIR_Norm_RW_NoAllocate 0
#define MAIR_Norm_RW_Allocate   1

#define MAIR_WriteThrough 0
#define MAIR_WriteBack    1

#define MAIR_Transient    0
#define MAIR_NonTransient 1

#define MAIR_Norm_RW(R,W) ((R&1)|((W&1)<<1))
#define MAIR_Norm_WrTyp(WrTyp) ((WrTyp)<<2)
#define MAIR_Norm_TrTyp(TrTyp) ((TrTyp)<<3)

#define MAIR_Norm_Cacheable(mem_attr_idx, R, W, WrTyp, TrnsTyp) \
  (((MAIR_Norm_RW(R,W)) & 2)|\
  (MAIR_Norm_WrTyp(WrTyp) & 1)|\
  (MAIR_Norm_TrTyp(TrnsTyp) & 1))

#define MAIR_NormReadonly(in) MAIR_Norm_Cacheable(mem_attr_idx, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_NoAllocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << (8 * mem_attr_idx)

#define MAIR_SetAttr_NonCacheable(mem_attr_idx) (0b0100 << (4 * mem_attr_idx))

#define MAIR_SetAttr_Cacheable(mem_attr_idx) MAIR_Norm_Cacheable(mem_attr_idx, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_Allocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << 4 * in

#define MAIR_SetAttr_Dev_nGnRE(mem_attr_idx) (MAIR_dd_nGnRnE << (8 * mem_attr_idx))

