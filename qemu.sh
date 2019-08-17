#!/bin/bash

#gdb -x /home/zombie/projects/raspberri/baremetal_aarch64/qemu_debug.gdb \
# --args \
 /root/qemu/bin/qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -s -d mmu,op,int,page
#  -p $(pidof qemu-system-aarch64)
