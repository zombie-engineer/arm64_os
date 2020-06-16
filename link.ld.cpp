#include <config.h>

#define PAGE_SIZE 4096
#define CPU_STATE_MAX_SIZE 512
#define STACKS_TOP(__el) __stacks_el ## __el ## _top
#define STACKS_BASE(__el) __stacks_el ## __el ## _base
#define STACKS_SIZE(__el) (1 << STACK_SIZE_EL ## __el ## _LOG)
#define STACKS_SECT_SIZE(__el) __stacks_el ## __el ## _size
#define STACKS_SECTION(__el)\
  . = ALIGN(0x10000);\
  .stacks.el ## __el : {}\
  STACKS_TOP(__el) = ADDR(.stacks.el ## __el);\
  STACKS_SECT_SIZE(__el) = (STACKS_SIZE(__el) * NUM_CORES);\
  . += STACKS_SECT_SIZE(__el);\
  STACKS_BASE(__el) = ADDR(.stacks.el ## __el) + STACKS_SECT_SIZE(__el);

#define CPUSTATE_SECTION \
  . += 0x1000;\
  . = ALIGN(PAGE_SIZE);\
  .init.cpustate : {}\
  __init_cpu_state = ADDR(.init.cpustate);\
  . += (CPU_STATE_MAX_SIZE * NUM_CORES);\

#define DMA_AREA_SECTION\
  . = ALIGN(PAGE_SIZE);\
  .dma_area : {}\
  __dma_area_start = ADDR(.dma_area);\
  __dma_area_end = __dma_area_start + DMA_AREA_SIZE;\
  . += DMA_AREA_SIZE;


SECTIONS
{
  . = 0x80000;
  __stack_base = .;
  .text :
  {
    KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*)
  }

  . = ALIGN(8);
  .rodata :
  {
    *(.rodata .rodata.* .gnu.linkonce.r*)
  }

  . = ALIGN(4096);
  PROVIDE(_data = .);
  .data : {*(.data .gnu.linkonce.d*) }
  .data.bin ALIGN(4): SUBALIGN(4) { *(.data.*) }

  .mybss (NOLOAD) :
  {
    . = ALIGN(16);
    *(COMMON)
    *(.bss)
    *(.bss.*)
  }
  __bss_start = ADDR(.mybss);
  __bss_end = __bss_start + SIZEOF(.mybss);
  STACKS_SECTION(0)
  STACKS_SECTION(1)
  CPUSTATE_SECTION
  DMA_AREA_SECTION

  . = ALIGN(16384);
  __mmu_table_base = .;

  /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = __bss_end - __bss_start;
__stack_size_el0_log = STACK_SIZE_EL0_LOG;
__stack_size_el1_log = STACK_SIZE_EL1_LOG;
__init_cpu_state_max_size = CPU_STATE_MAX_SIZE;
