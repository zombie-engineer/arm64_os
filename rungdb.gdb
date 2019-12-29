set breakpoint pending on
target remote localhost:1234
file kernel8.elf

b _start
b main
# b wait_timer
# b __handle_interrupt
# b bcm2835_systimer_set
b __interrupt_cur_el_spx_irq
displ/8i $pc
displ/8gx $sp - 0x40
displ/8gx $sp
c 


