set breakpoint pending on
target remote localhost:1234
file kernel8.elf
b _vsprintf
b _sprintf
b printf
#b to_char_16
b spi_work
# b mmu_init
# displ/8i $pc
b textviewport_scrolldown
b rectangle.c:89
# c 


