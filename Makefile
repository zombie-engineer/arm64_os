GDB := 

CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy

OBJS := \
	binblock.o\
	console.o\
	delays.o\
	gpio.o\
	power.o\
	rectangle.o\
	shiftreg.o\
	unhandled_exception.o\
	viewport_console.o\
	clock_manager.o\
	cpuctx_generic.o\
	dma.o\
	intr_ctl.o\
	pwm.o\
	ringbuf.o\
	tags.o\
	vcanvas.o\
	common.o	\
	debug.o		 \
	exception.o\
	rand.o\
	sched.o\
	timer.o\
	video_console.o

INCLUDES := include

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

OBJS += $(LIBS) font_bin.o to_raspi.nokia5110.o font/font.o lib/checksum.o

font_bin.o: font.psf
	$(LD) -r -b binary $< -o $@

to_raspi.nokia5110.d font_bin.d: font.psf
	echo $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c %.d
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: main_qemu.o
main_qemu.o: main.c
	echo $<
	$(CC) $(CFLAGS) -DCONFIG_QEMU -c $< -o $@


.PHONY: to_raspi.nokia5110.o
to_raspi.nokia5110.o: 
	$(LD) -r -b binary to_raspi.nokia5110 -o $@

kernel8.img: $(OBJS) main.o
	$(LD) -nostdlib -nostartfiles $^ -T arch/armv8/link.ld -o $(@:.img=.elf) -Map $(@:.img=.map)
	$(NM) --numeric-sort kernel8.elf > $@.sym
	$(OBJCOPY) -O binary kernel8.elf $@

qemubin.img: $(OBJS) main_qemu.o
	$(LD) -nostdlib -nostartfiles $^ -T arch/armv8/link.ld -o $@.elf -Map $@.map
	$(NM) --numeric-sort $@.elf > $@.sym
	$(OBJCOPY) -O binary $@.elf $@


QEMU_FLAGS := -M raspi3 -accel tcg -nographic
QEMU_TRACE_ARGS := -trace enable=*bcm2835*

TRACE_QEMU := 0
ifeq ($(TRACE_QEMU), 1)
QEMU_FLAGS += $(QEMU_TRACE_FLAGS)
endif

# Run in qemu
.PHONY: qemu
qemu: qemubin.img
	$(QEMU) $(QEMU_FLAGS) -kernel $<

# Run in qemu, wait until debugger attached
.PHONY: qemud
qemud: qemubin.img
	$(QEMU) $(QEMU_FLAGS) -kernel $< -s -S

# Run in qemu, with qemu also debugged AND wait until debugger attached
# Note: qemu.gdb holds script that will be executed while debugging
# qemu process. Other gdb also needed to be attached externally via
# -s -S flags for a normal debug session.
.PHONY: qemuds
qemuds: qemubin.img
	gdb -x qemu.gdb\
		--args $(QEMU) $(QEMU_FLAGS) -kernel $< -s -S

# Attach to started qemu process with gdb
.PHONY: qemuat
qemuat:
	/home/zombie/binutils-gdb/gdb/gdb -x rungdb.gdb

.PHONY: serial
serial:
	minicom -b 115200 -D /dev/ttyUSB0 -C uart_capture_$$(date +%S).bin

.PHONY: clean
clean:
	find -name '*.o' -exec rm -v {} \;
	find -name '*.d' -exec rm -v {} \;

DEPS := $(OBJS:.o=.d)

$(info $(DEPS))

%.d: %.c
	cpp -M -I$(INCLUDES) $< > $@

%.d: %.S
	cpp -M -I$(INCLUDES) $< > $@

-include $(DEPS)

.PHONY: all
all: $(DEPS) kernel8.img 
