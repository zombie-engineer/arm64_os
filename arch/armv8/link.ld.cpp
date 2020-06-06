#include <config.h>

#define STACKS_TOP(__el) __stacks_el ## __el ## _top
#define STACKS_BASE(__el) __stacks_el ## __el ## _top
#define STACKS_SIZE(__el) (STACK_SIZE_EL ## __el)
#define STACKS_SECTION(__el)\
  .stacks.el ## __el :\
  {\
    STACKS_TOP(__el) = .;\
    . += STACKS_TOP(__el) + (STACKS_SIZE(__el) * NUM_CORES);\
    STACKS_BASE(__el) = .;\
  }

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
    __bss_start = .;
    *(COMMON)
    *(.bss)
    *(.bss.*)
    __bss_end = .;
  }
  . = ALIGN(4096);
  _end = .;
  . = ALIGN(4096);

  STACKS_SECTION(1)
  STACKS_SECTION(0)

  . = ALIGN(4096);
  __bin_start = .;

  . = ALIGN(16384);
  __mmu_table_base = .;

  /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = __bss_end - __bss_start;
