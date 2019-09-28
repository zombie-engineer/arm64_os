#pragma once

// ARMv8-A TCR control register

typedef struct {
  char T0SZ    : 6;
  char RES0_6  : 1;
  // Translation table walk disable for translations using TTBR0_EL1. This bit controls whether a
  // translation table walk is performed on a TLB miss, for an address that is translated using
  // TTBR0_EL1
  char EPD0    : 1;
  // Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL1.
  char IRGN0   : 2;
  // Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL1.
  char ORGN0   : 2;
  // Shareability attribute for memory associated with translation table walks using TTBR0_EL1.
  char SH0     : 2;
  // Granule size for the TTBR0_EL1.
  char TG0     : 2;
  // The size offset of the memory region addressed by TTBR1_EL1
  // region size is pow(2, 64 - T1SZ)
  char T1SZ    : 6;
  // Selects whether TTBR0_EL1 or TTBR1_EL1 defines the ASID
  char A1      : 1;
  // Translation table walk disable for translations using TTBR1_EL1. This bit controls whether a
  // translation table walk is performed on a TLB miss, for an address that is translated using
  // TTBR1_EL1
  char EPD1    : 1;
  // Inner cacheability attribute for memory associated with translation table walks using TTBR1_EL1.
  char IRGN1   : 2;
  // Outer cacheability attribute for memory associated with translation table walks using TTBR1_EL1.
  char ORGN1   : 2;
  // Shareability attribute for memory associated with translation table walks using TTBR1_EL1.
  char SH1     : 2;
  // Granule size for the TTBR1_EL1.
  char TG1     : 2;
  // Intermediate Physical Address Size.
  char IPS     : 3;
  char RES0_35 : 1;
  // ASID Size
  char AS      : 1;
  // Top Byte ignored - indicates whether the top byte of an address is used for address match for the
  // TTBR1_EL1 region, or ignored and used for tagged addresses.
  char TBI0    : 1;
  // Top Byte ignored - indicates whether the top byte of an address is used for address match for the
  // TTBR1_EL1 region, or ignored and used for tagged addresses.
  char TBI1    : 1;
  // Hardware Access flag update in stage 1 translations from EL0 and EL1.
  char HA      : 1;
  // Hardware management of dirty state in stage 1 translations from EL0 and EL1.
  char HD      : 1;
  // Hierarchical Permission Disables. This affects the hierarchical control bits, APTable, PXNTable,
  // and UXNTable, except NSTable, in the translation tables pointed to by TTBR1_EL1.
  char HPD0    : 1;
  // Hierarchical Permission Disables. This affects the hierarchical control bits, APTable, PXNTable,
  // and UXNTable, except NSTable, in the translation tables pointed to by TTBR1_EL1.
  char HPD1    : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[59] of the stage 1
  char HWU059  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[60] of the stage 1
  char HWU060  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[61] of the stage 1
  char HWU061  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[62] of the stage 1
  char HWU062  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[59] of the stage 1
  char HWU159  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[60] of the stage 1
  char HWU160  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[61] of the stage 1
  char HWU161  : 1;
  // Hardware Use. Indicates IMPLEMENTATION DEFINED hardware use of bit[62] of the stage 1
  // translation table Block or Page entry for translations using TTBR1_EL1.
  char HWU162  : 1;
  // Controls the use of the top byte of instruction addresses for address matching.
  char TBID0   : 1;
  // Controls the use of the top byte of instruction addresses for address matching.
  char TBID1   : 1;
  // Non-fault translation table walk disable for stage 1 translations using TTBR0_EL1.
  char NFD0    : 1;
  // Non-fault translation table walk disable for stage 1 translations using TTBR1_EL1.
  char NFD1    : 1;
  // Faulting control for EL0 access to any address translated by TTBR1_EL1
  char EOPD0   : 1;
  // Faulting control for EL0 access to any address translated by TTBR1_EL1
  char EOPD1   : 1;
  // Controls the generation of Unchecked accesses at EL1, and at EL0
  char TCMA0   : 1;
  // Controls the generation of Unchecked accesses at EL1, and at EL0
  char TCMA1   : 1;
  char RES0_63 : 5;
} __attribute__((packed)) tcr_el1_t;

#define TCR_IPS_32_BITS 0b000 
#define TCR_IPS_36_BITS 0b001
#define TCR_IPS_40_BITS 0b010
#define TCR_IPS_42_BITS 0b011
#define TCR_IPS_44_BITS 0b100
#define TCR_IPS_48_BITS 0b101
#define TCR_IPS_52_BITS 0b110

#define TCR_TG_4K       0b10
#define TCR_TG_16K      0b01
#define TCR_TG_64K      0b11

#define TCR_SH_NON_SHAREABLE   0b00
#define TCR_SH_OUTER_SHAREABLE 0b10
#define TCR_SH_INNER_SHAREABLE 0b11

#define TCR_RGN_NON_CACHEABLE 0b00
#define TCR_RGN_WbRaWaC       0b01
#define TCR_RGN_WtRaWnC       0b10
#define TCR_RGN_WbRaWnC       0b11

#define TCR_EPD_WALK_ENABLED 0
#define TCR_EPD_WALK_DISABLE 1


