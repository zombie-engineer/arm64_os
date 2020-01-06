set breakpoint pending on
set pagination off
target remote localhost:1234
file kernel8.elf

b _start
b __armv8_cpuctx_eret_to_proc
# b switch_to_initial_task
# b __interrupt_cur_el_spx_irq
b * __armv8_cpuctx_eret+84
displ/8i $pc
displ/8gx $sp - 0x40
displ/8gx $sp
c 


