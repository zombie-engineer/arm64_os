set breakpoint pending on
target remote localhost:1234
file kernel8.elf
b irq_handle_generic
b system_timer_set
b systimer_init
b wait_timer
displ/8i $pc
#c 


