GDB := 
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
INCLUDES = include

OPTIMIZATION_FLAGS = -O2
OPTIMIZATION_FLAGS = -g

CFLAGS = -Wall $(OPTIMIZATION_FLAGS) -ffreestanding -nostdinc -nostdlib -nostartfiles -I$(INCLUDES)
CROSS_COMPILE = /home/zombie/projects/crosscompile/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
QEMU := /home/zombie/qemu/aarch64-softmmu/qemu-system-aarch64

all: kernel8.img

include uart/Makefile
include spi/Makefile
include mbox/Makefile
include arch/armv8/Makefile
include cmdrunner/Makefile
include drivers/max7219/Makefile
include drivers/f5161ah/Makefile
include drivers/nokia5110/Makefile
include board/bcm2835/Makefile
include lib/stringlib/Makefile


LIBS = 
LIBS += $(OBJS_UART) 
LIBS += $(OBJS_MBOX) 
LIBS += $(OBJS_ARMV8) 
LIBS += $(OBJS_CMDRUNNER) 
LIBS += $(OBJS_MAX7219)
LIBS += $(OBJS_F5161AH)
LIBS += $(OBJS_SPI)
LIBS += $(OBJS_NOKIA5110)
LIBS += $(OBJS_BOARD_BCM2835)
LIBS += $(OBJS_STRINGLIB)
$(info LIBS = $(LIBS))

OBJS += $(LIBS) font.o to_raspi.nokia5110.o

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy


%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

font.o: font.psf
	$(LD) -r -b binary -o font.o font.psf

.PHONY: to_raspi.nokia5110.o
to_raspi.nokia5110.o: 
	$(LD) -r -b binary to_raspi.nokia5110 -o $@

kernel8.img: $(OBJS)
	$(LD) -nostdlib -nostartfiles $(OBJS) -T arch/armv8/link.ld -o kernel8.elf -Map kernel8.map
	$(OBJCOPY) -O binary kernel8.elf $@

run:
	$(QEMU) -M raspi3 -kernel kernel8.img -nographic -s

rungdb:
	$(QEMU) -M raspi3 -kernel kernel8.img -nographic -s -S


#		-ex 'b bcm2835_ic_write'\
#		-ex 'b bcm2835_timer_tick_common'\
#		-ex 'b bcm2835_peripherals_init'\
#	 	-ex 'b bcm2835_peripherals_realize'\
#		-ex 'b el_from_spsr' \
#		-ex 'b helper_exception_return' 
#
rungdb2:
	gdb -x qemu.gdb\
	 	--args $(QEMU) -M raspi3 -kernel kernel8.img -nographic -s -S

rungdbq:
	./qemu.sh

#	gdb-multiarch -x rungdb.gdb
gdb:
	/home/zombie/binutils-gdb/gdb/gdb -x rungdb.gdb

serial:
	minicom -b 115200 -D /dev/ttyUSB0

.PHONY: clean
clean:
	find -name '*.o' -exec rm -v {} \;

