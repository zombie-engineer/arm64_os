set breakpoint pending on
target remote localhost:1234
file kernel8.elf
b main
b mmu_init
displ/8i $pc