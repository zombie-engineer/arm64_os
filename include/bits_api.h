#pragma once

#define BIT(x) (1UL << (x))

#define BITWIDTH64(x) ((long long)x)

#define BITS_AT_POS(bits, pos, mask) ((BITWIDTH64(bits) & mask) << pos)

#define BIT_AT_POS(bit, pos) BITS_AT_POS(bit, pos, 1)

#define BITWIZE_OR(a, b) (a | b)

#define BT(bitpos) (1<<(bitpos))
#define BT_V(bitpos, bitval) ((bitval&1)<<(bitpos))

#define BIT_IS_SET(val, bitpos) (val & BT(bitpos))

#define BIT_IS_CLEAR(val, bitpos) (!BIT_IS_SET(val, bitpos))

#define BIT_CLEAR(val, bitpos) val &= ~BT(bitpos)

#define BIT_CLEAR_U32(val, bitpos) val &= ~BT(bitpos)

#define BIT_SET(val, bitpos) val |= BT(bitpos)

#define BIT_SET_U32(val, bitpos) val |= BT(bitpos)


#define BIT_SET_V_8(val, bitpos, bitval) val |= BT_V(bitpos, bitval)

#define BF_SHIFT(val, offset) ((val)<<offset)

#define BF_MASK_32(width) ((uint32_t)(BF_SHIFT((uint64_t)1, width)-1))

#define BF_MASK_AT_32(offset, width) (BF_MASK_32(width)<<offset)

#define BITS_PLACE(type, val, width, offset) ((type)BITS_AT_POS(val, offset, BF_MASK_32(width)))

#define BITS_PLACE_32(val, offset, width) (BITS_PLACE(uint32_t, val, width, offset))

#define BF_EXTRACT(val, offset, width) ((val>>offset) & BF_MASK_32(width))

#define BF_CLEAR(val, offset, width) val &= ~BF_MASK_AT_32(offset, width)

#define BF_ORR(val, set, offset, width)\
  val |= BITS_PLACE_32(set, offset, width)

#define BF_CLEAR_AND_SET(val, set, offset, width) \
  BF_CLEAR(val, offset, width);\
  BF_ORR(val, set, offset, width);

#define BYTE_EXTRACT(__v, __i) ((char)((__v >> (__i * 8)) & 0xff))

