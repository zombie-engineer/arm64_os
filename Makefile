SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
INCLUDES = include

OPTIMIZATION_FLAGS = -O2
OPTIMIZATION_FLAGS = -g

CFLAGS = -Wall $(OPTIMIZATION_FLAGS) -ffreestanding -nostdinc -nostdlib -nostartfiles -I$(INCLUDES)
CROSS_COMPILE = /home/zombie/projects/crosscompile/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-

all: clean kernel8.img

include uart/Makefile
include spi/Makefile
include mbox/Makefile
include arch/armv8/Makefile
include cmdrunner/Makefile
include drivers/max7219/Makefile


LIBS = 
LIBS += $(OBJS_UART) 
LIBS += $(OBJS_MBOX) 
LIBS += $(OBJS_ARMV8) 
LIBS += $(OBJS_CMDRUNNER) 
LIBS += $(OBJS_MAX7219)
LIBS += $(OBJS_SPI)
$(info LIBS = $(LIBS))

OBJS += $(LIBS) font.o start.o

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

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

make serial:
	minicom -b 115200 -D /dev/ttyUSB0
