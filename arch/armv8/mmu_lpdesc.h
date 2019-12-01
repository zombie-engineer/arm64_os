#pragma once


// Access from higher EL - Read/Write, From Access from EL0 - None
#define MMU_S1_AP_RW_NO 0
// Access from higher EL - Read/Write, From Access from EL0 - Read/Write
#define MMU_S1_AP_RW_RW 1
// Access from higher EL - Read-only, From Access from EL0 - None
#define MMU_S1_AP_RO_NO 2
// Access from higher EL - Read-only, From Access from EL0 - Read-only
#define MMU_S1_AP_RO_RO 3

// Translation table descriptor for level 0, level 1 and level 2 for Aarch64

 //typedef struct {
 //  // bit[0], valid bit - indicates this whole descriptor is valid or not.
 //  //                     in case it's not translation fault is generated during page 
 //  //                     walk that arrived at this descriptor.
 //  char     valid_bit      : 1;
 //
 //  // bit[1], pagetable/block pointer bit - a value of 1 means this descriptor points
 //  //                     to a page table, otherwise it points to a page block
 //  char     page_block_bit : 1;
 //
 //
 //  // lower attributes
 //  // bits[4:2], index into memory region attrubite register (see MAIR description)
 //  char AttrIndx  : 3;
 //
 //  // bit[5], NS - Non-Secure bit. For memory accesses from Secure state, specifies
 //  //              wheather the output address is in Secure or Non-Secure address map
 //  char NS        : 1;
 //
 //  // bits[7:6], AP - Data Access Permissions. Data access permission controls on page D5-2580.
 //  //                 Use MMU_S1_AP_* macros for values
 //  char AP        : 2;
 //
 //  // bits[9:8], SH - Shareability field
 //  char SH        : 2;
 //
 //  // bit[10], AF - Access flag
 //  char AF        : 1;
 //
 //  // bit[11], nG - Not global bit.
 //  char nG        : 1;
 //
 //  uint64_t address        : 36;
 //  char     res0_48        : 3;
 //  char     ign_52         : 7;
 //  char     PXNTable       : 1;
 //  char     XNTable        : 1;
 //  char     APTable        : 2;
 //  char     NSTable        : 1;
 //
 //} __attribute__((packed)) vmsav8_64_block_dsc_t;
