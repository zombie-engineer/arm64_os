set pagination off
delete

# b bcm2835_systmr_update_compare
# commands
# silent
# b qemu_clock_get_us
# printf "bcm2835_systmr_update_compare\n"
# c
# end

# b trace_opt_parse
# b bcm2835_ic_set_gpu_irq
# b bcm2835_ic_write
# b bcm2836_control_write
b pl011_realize

b timer_expired_ns
commands
silent
printf "timer_expired_ns: current_time: %lu", current_time
if timer_head
  printf ", expire_time: %lu", timer_head->expire_time
end
printf "\n"
c
end

r

# b arm_cpu_do_interrupt_aarch64

# b qemu_clock_run_all_timers
# commands
# silent 
# printf "qemu_clock_run_all_timers\n"
# c
# end

# b timer_expired
# commands
# silent
# printf "timer_expired\n"
# c
# end
# 
# b timer_mod
# commands
# silent
# printf "timer_mod\n"
# c
# end
# 
# b timer_del
# commands
# silent
# printf "timer_del\n"
# c
# end
# 
# b bcm2835_systmr_init
# commands
# silent
# printf "bcm2835_systmr_init\n"
# c
# end
# 
# b bcm2835_systmr_realize
# commands
# silent
# printf "bcm2835_systmr_realize\n"
# c
# end
# 
# b timerlist_new 
# commands
# silent
# printf "timerlist_new\n"
# c
# end
# 
# b qemu_clock_init
# commands
# silent
# printf "qemu_clock_init\n"
# c
# end
# 
# b qemu_clock_notify
# commands
# silent
# printf "qemu_clock_notify\n"
# c
# end
# 
# b qemu_clock_enable
# commands
# silent
# printf "qemu_clock_enable\n"
# c
# end
# 
# b timerlist_expired
# commands
# silent
# printf "timerlist_expired\n"
# c
# end
# 
# b qemu_clock_deadline_ns_all
# commands
# silent
# printf "qemu_clock_deadline_ns_all\n"
# c
# end
# 
# b qemu_soonest_timeout
# commands
# silent
# printf "qemu_soonest_timeout\n"
# c
# end
# 
# b timerlist_get_clock 
# commands
# silent
# printf "timerlist_get_clock\n"
# c
# end
# 
# b timerlist_notify
# commands
# silent
# printf "timerlist_notify\n"
# c
# end
# 
# b timer_init_full
# commands
# silent
# printf "timer_init_full\n"
# c
# end
# 
# b timerlist_rearm
# commands
# silent
# printf "timerlist_rearm\n"
# c
# end



#		-ex 'b helper_exception_return' \
#		-ex 'b timerlist_run_timers' \
# 	 	-ex 'b bcm2835_systmr_update_compare' \
