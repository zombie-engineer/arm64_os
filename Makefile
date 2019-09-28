SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
INCLUDES = include
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -I$(INCLUDES)
CFLAGS = -Wall -g -ffreestanding -nostdinc -nostdlib -nostartfiles -I$(INCLUDES)
CROSS_COMPILE = /home/zombie/projects/crosscompile/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

all: clean kernel8.img

include uart/Makefile
include mbox/Makefile

OBJS += $(OBJS_UART) $(OBJS_MBOX)
OBJS += font.o start.o armv8.o

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

armv8.o: armv8.S
	$(CC) $(CFLAGS) -c $? -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

font.o: font.psf
	$(LD) -r -b binary -o font.o font.psf

kernel8.img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $(OBJS) -T link.ld -o kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf $@

clean:
	rm kernel8.elf *.o > /dev/nell 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -s

rungdb:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -s -S

rungdbq:
	./qemu.sh

gdb:
	gdb-multiarch -x rungdb.gdb
