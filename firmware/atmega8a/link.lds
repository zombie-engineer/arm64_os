SECTIONS 
{
  . = 0;
  .text    : { start.o(.text) *(.text*) }
}
