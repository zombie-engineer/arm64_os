CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles
CROSS_COMPILE = /home/zombie/projects/crosscompile/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

all: clean kernel8.img

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

kernel8.img: start.o
	$(LD) -nostdlib -nostartfiles start.o -T link.ld -o kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf *.o > /dev/nell 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -d in_asm
