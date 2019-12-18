#pragma once

#define BITWIDTH64(x) ((long long)x)

#define BITS_AT_POS(bits, pos, mask) ((BITWIDTH64(bits) & mask) << pos)

#define BIT_AT_POS(bit, pos) BITS_AT_POS(bit, pos, 1)

#define BITWIZE_OR(a, b) (a | b)
