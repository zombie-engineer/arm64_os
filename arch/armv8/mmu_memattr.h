#pragma once

/* Memory region attributes encoding */

#define MAIR_dd_nGnRnE (0 << 2)
#define MAIR_dd_nGnRE  (1 << 2)
#define MAIR_dd_nGRE   (2 << 2)
#define MAIR_dd_GRE    (3 << 2)

#define MAIR_Norm_RW_NoAllocate 0
#define MAIR_Norm_RW_Allocate   1

#define MAIR_WriteThrough 0
#define MAIR_WriteBack    1

#define MAIR_Transient    0
#define MAIR_NonTransient 1

#define MAIR_Norm_RW(R,W) ((R&1)|((W&1)<<1))
#define MAIR_Norm_WrTyp(WrTyp) ((WrTyp)<<2)
#define MAIR_Norm_TrTyp(TrTyp) ((TrTyp)<<3)

#define MAIR_Norm_Cacheable(in, R, W, WrTyp, TrnsTyp) \
  (((MAIR_Norm_RW(R,W)) & 2)|\
  (MAIR_Norm_WrTyp(WrTyp) & 1)|\
  (MAIR_Norm_TrTyp(TrnsTyp) & 1))

#define MAIR_Norm_NonCacheable(in) (0b0100 << 4 * in)

#define MAIR_Norm(in) MAIR_Norm_Cacheable(in, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_Allocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << 4 * in

#define MAIR_NormReadonly(in) MAIR_Norm_Cacheable(in, \
  MAIR_Norm_RW_Allocate,\
  MAIR_Norm_RW_NoAllocate,\
  MAIR_WriteBack,\
  MAIR_NonTransient) << 4 * in

#define MAIR_normal(iiii, oooo) (((iiii)&0xf)<<4|((oooo)&0xf))
