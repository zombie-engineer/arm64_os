#pragma once

#define DECL_PERIPH_BASE(type, var, addr) volatile type * var = (volatile type *)(addr)
