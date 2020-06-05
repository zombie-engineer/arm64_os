set breakpoint pending on
set pagination off
target remote localhost:1234
file kernel8.elf

define dctx
p/x *(armv8_cpuctx_t *)__current_cpuctx
end

# b intr_ctl_set_cb
# b intr_ctl_arm_irq_enable
b __handle_interrupt
commands
silent
printf "__handle_interrupt\n"
bt
c
end

b main
# b _start
# b switch_to_initial_task
# b __interrupt_cur_el_spx_irq
# b * __armv8_cpuctx_eret+84
b pl011_uart_handle_interrupt
b pl011_io_thread
b scheduler_init
displ/8i $pc
displ/8gx $sp - 0x40
displ/8gx $sp
c 


