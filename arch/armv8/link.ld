
SECTIONS
{
  . = 0x80000;
  __stack_base = .;
  .text : 
  { 
    KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) 
  }

  . = ALIGN(8);
  __arm_initial_reg_value_address_hcr_el2 = .;
  .rodata : 
  { 
    *(.rodata .rodata.* .gnu.linkonce.r*) 
  }

  . = ALIGN(4096);
  PROVIDE(_data = .);
  .data : {*(.data .data.* .gnu.linkonce.d*) }
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

  /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }

  . = ALIGN(4096);
  __bin_start = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
/* https://github.com/bztsrc/raspi3-tutorial/blob/master/02_multicorec/link.ld */

