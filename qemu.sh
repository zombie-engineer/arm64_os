#!/bin/bash

gdb -x /home/zombie/projects/raspberry/baremetal_aarch64/qemu_debug.gdb \
 --args \
 qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -s -S -d mmu,op,int,page
#  -p $(pidof qemu-system-aarch64)
