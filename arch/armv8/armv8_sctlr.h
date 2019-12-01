#pragma once

// ARMv8-A SCTLR control register

// Provides top level control of the system, including its memory system, at EL1 and EL0
// typedef struct {
//   // bit [0], el1, el0 address translation enable/disable 0-disable, 1-enable
//   char M       : 1;  
// 
//   // bit [1], alignment check enable 0-disable, 1-enable
//   char A       : 1;
// 
//   // bit [2], cacheability control for data accessess
//   //          0-all data access is non-cacheable
//   //          1-all data access is norml
//   char C       : 1;
// 
//   // bit [3], SP alignment check enable, generate fault ex at load store if SP not 16byte- aligned
//   char SA      : 1;
// 
//   // bit [4], SP alignment check enable for EL0
//   char SA0     : 1;
// 
//   // bit [5], system instruction memory barrier enable. 0-disabled, 1-enabled
//   char CP15BEN : 1;
// 
//   // bit [6], non-aligned access
//   char nAA     : 1; 
// 
//   // bit [7], IT Disable
//   char ITD     : 1;
// 
//   // bit [8], SETEND isntruction disable
//   char SED     : 1;
// 
//   // bit [9], User Mask Access. Traps EL0 execution of MSR/MRS instructions,
//   // that access PSTATE
//   char UMA     : 1;
// 
//   // bit [10]
//   char EnRCTX  : 1;
// 
//   // bit [11], exception Exit is Context Synchronizing
//   char EOS     : 1;
// 
//   // bit [12], instruction access Cacheability control for EL0, EL1
//   char I       : 1;
// 
//   // bit [13], enable pointer authentication
//   char EnDB    : 1;
// 
//   // bit [14], trap DC ZVA at EL0 to EL1 or EL2
//   char DZE     : 1;
// 
//   // bit [15], traps CTR_EL0 to EL1 or EL2
//   char UCT     : 1;
// 
//   // bit [16], traps WFI from EL0 to EL1 or EL2
//   char nTWI    : 1;
// 
//   // bit [17] 
//   char RES0_17 : 1;
// 
//   // bit [18], traps WFE
//   char nTWE    : 1;
// 
//   // bit [19], write permission implies XN (Execute-never)
//   //           1-Any region that is writable in the EL1&0 translation regime is forced to XN for accesses
//   //                 from software executing at EL1 or EL0
//   //           0-not
//   char WXN     : 1;
// 
//   // bit [20]
//   char TSCXT   : 1;
//   // Implicit Error Synchronization event enable
//   char IESB    : 1;
//   // Exception Entry is Context Synchronizing
//   char EIS     : 1;
//   // Set Privileged Access Never on taking an exception to EL1
//   char SPAN    : 1;
//   // Endianness of data accesses at EL0 0-little, 1-big
//   char EOE     : 1;
//   // Endianness of data accesses at EL1 and stage 1 translation table walks
//   // in EL1&0 regime 0-little, 1-big
//   char EE      : 1;
//   // Traps EL0 execution of cache maintenance instructions
//   char UCI     : 1;
//   // Controls enabling of pointer authentication of instruction addresses
//   // in EL1&0 translation regime
//   char EnDA    : 1;
//   // No Trap Load Multiple and Store Multiple to Device-nGRE/Device-nGnRE/Device-nGnRnE
//   // memory
//   char nTLSMD  : 1;
//   // Load Multiple and Store Multiple Atomicity and Ordering Enable
//   char LSMAOE  : 1;
//   // Controls enabling of pointer authentication of instriction addresses 
//   // in EL1&0 translation regime
//   char EnIB    : 1;
//   // Controls enabling of pointer authentication of instriction addresses 
//   // in EL1&0 translation regime
//   char EnIA    : 1;
//   // 
//   char Res0_34 : 4;
//   // PAC Branch Type compatibility at EL0
//   char BT0     : 1;
//   // PAC Branch Type compatibility at EL1
//   char BT1     : 1;
//   // 
//   char ITFSB   : 1;
//   // Tag Check Fail in EL0
//   char TCF0    : 2;
//   // Tag Check Fail in EL1
//   char TCF     : 2;
//   // Allocation Tag Access in EL0
//   char ATA0    : 1;
//   // Allocation Tag Access in EL1
//   char ATA     : 1;
//   // Default PSTATE.SSBS
//   char DSSBS   : 1;
//   // 
//   int RES0_45 : 18;
// } __attribute__((packed)) sctlr_el1_t;

