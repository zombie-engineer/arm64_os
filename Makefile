CROSS_COMPILE = ~/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf/bin/aarch64-none-elf-

CC      = $(CROSS_COMPILE)gcc
CPP     = $(CROSS_COMPILE)cpp
LD      = $(CROSS_COMPILE)ld
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
REGTOOL = tools/gen_reg_headers.py
GDB     = /mnt/sdb1/binutils-gdb/gdb/gdb

INCLUDES := include firmware/atmega8a/include

OPTIMIZATION_FLAGS = -O2
OPTIMIZATION_FLAGS = -g

LINKSCRIPT = link.ld

WARNINGS_AS_ERR := \
	-Werror=return-type\
	-Werror=unused-label\
	-Werror=uninitialized\
	-Werror=incompatible-pointer-types\
	-Werror=unused-variable\
	-Werror=int-conversion\
	-Werror=implicit-function-declaration\
	-Werror=shift-count-overflow\
	-Werror=unused-value\
	-Wparentheses

CFLAGS = $(WARNINGS_AS_ERR) -Wall $(OPTIMIZATION_FLAGS) -ffreestanding -nostdinc -nostdlib -nostartfiles $(INCLUDES_FLAGS)
# CFLAGS += -mstrict-align
LDFLAGS = -nostdlib -T $(LINKSCRIPT)
# LDFLAGS = -nostdlib -nostartfiles -T $(LINKSCRIPT)
QEMU := qemu-system-aarch64
# QEMU := /home/zombie/qemu/aarch64-softmmu/qemu-system-aarch64

OBJS := \
	binblock.o\
	clock_manager.o\
	common.o\
	console.o\
	cpuctx_generic.o\
	debug.o\
	delays.o\
	dma_memory.o\
	dma.o\
	exception.o\
	exception_reporter.o\
	gpio_set.o\
	init_task.o\
	intr_ctl.o\
	irq.o\
	jtag.o\
	kernel_panic.o\
	mutex.o\
	kmalloc.o\
	percpu.o\
	pipe.o\
	power.o\
	pwm.o\
	rand.o\
	rectangle.o\
	ringbuf.o\
	sched.o\
	self_test.o\
	semaphore.o\
	shiftreg.o\
	tags.o\
	timer.o\
	unhandled_exception.o\
	vcanvas.o\
	viewport_console.o\
	video_console.o\
	kernel_tests/test_dma.o

ifeq (CONFIG_AVR_UPDATER, y)
	OBJS += avr_update.o
endif

all: kernel8.img

include arch/armv8/Makefile
include bins/Makefile
include board/bcm2835/Makefile
include cmdrunner/Makefile
include drivers/max7219/Makefile
# include drivers/atmega8a/Makefile
include drivers/f5161ah/Makefile
# include drivers/nokia5110/Makefile
include drivers/tft_lcd/Makefile
include drivers/usbd/Makefile
include drivers/servo/sg90/Makefile
include fs/Makefile
include mbox/Makefile
include lib/stringlib/Makefile
include spi/Makefile
include uart/Makefile

$(info INCLUDES = "$(INCLUDES)")
INCLUDES_FLAGS = $(addprefix -I,$(INCLUDES))

LIBS =
LIBS += $(OBJS_UART)
LIBS += $(OBJS_MBOX)
LIBS += $(OBJS_ARMV8)
LIBS += $(OBJS_CMDRUNNER)
LIBS += $(OBJS_MAX7219)
LIBS += $(OBJS_F5161AH)
LIBS += $(OBJS_FS)
LIBS += $(OBJS_SPI)
LIBS += $(OBJS_NOKIA5110)
LIBS += $(OBJS_TFT_LCD)
LIBS += $(OBJS_USBD)
LIBS += $(OBJS_ATMEGA8A)
LIBS += $(OBJS_BOARD_BCM2835)
LIBS += $(OBJS_STRINGLIB)
LIBS += $(OBJS_BINS)
LIBS += $(OBJS_SERVO_SG90)
$(info LIBS = $(LIBS))

OBJS += $(LIBS) lib/checksum.o font/font.o lib/parse_string.o

TARGET_PREFIX_REAL := kernel8
TARGET_PREFIX_QEMU := kernel8_qemu

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: $(TARGET_PREFIX_QEMU).o
$(TARGET_PREFIX_QEMU).o: $(TARGET_PREFIX_QEMU).c
	$(CC) $(CFLAGS) -DCONFIG_QEMU -c $< -o $@

$(TARGET_PREFIX_QEMU).c: $(TARGET_PREFIX_REAL).c
	cp -fv $^ $@

%.o: %.c %.d
	$(CC) $(CFLAGS) -c $< -o $@

.SECONDARY: $(TARGET_PREFIX_REAL).o $(TARGET_PREFIX_QEMU).o $(OBJS)

%.elf: $(OBJS) $(BINOBJS) %.o
	echo $(LD) $(LDFLAGS) --warn-section-align -o $@ -Map $(@:.elf=.map) $^
	$(LD) $(LDFLAGS) --warn-section-align -o $@ -Map $(@:.elf=.map) $^

$(LINKSCRIPT): $(LINKSCRIPT).cpp
	$(CPP) -Iinclude $< | grep -v '^#' > $(LINKSCRIPT)

TARGET_QEMU_IMG := $(TARGET_PREFIX_QEMU).img

%.sym: %.elf
	$(NM) --numeric-sort $< > $@

%.img: %.elf
	$(OBJCOPY) -O binary $< $@

QEMU_FLAGS := -M raspi3 -accel tcg -nographic
QEMU_TRACE_ARGS := -trace enable=*bcm2835*

TRACE_QEMU := 0
ifeq ($(TRACE_QEMU), 1)
QEMU_FLAGS += $(QEMU_TRACE_FLAGS)
endif

# Run in qemu
.PHONY: qemu
qemu: $(TARGET_QEMU_IMG)
	$(QEMU) $(QEMU_FLAGS) -kernel $<

# Run in qemu, wait until debugger attached
.PHONY: qemud
qemud: $(TARGET_QEMU_IMG)
	$(QEMU) $(QEMU_FLAGS) -kernel $< -s -S

# Run in qemu, with qemu also debugged AND wait until debugger attached
# Note: qemu.gdb holds script that will be executed while debugging
# qemu process. Other gdb also needed to be attached externally via
# -s -S flags for a normal debug session.
.PHONY: qemuds
qemuds: $(TARGET_QEMU_IMG)
	gdb -x qemu.gdb\
		--args $(QEMU) $(QEMU_FLAGS) -kernel $< -s -S

# Attach to started qemu process with gdb
.PHONY: qemuat
qemuat:
	$(GDB) -x rungdb_qemu.gdb

# JTAG
.PHONY: j
j:
	/mnt/ssd240/openocd-code/src/openocd -f openocd-jtag.cfg --log_output openocd-jtag.log &
# with debug trace	/mnt/ssd240/openocd-code/src/openocd -d -f openocd-jtag.cfg

# JTAG gdb
.PHONY: jd
jd:
	$(GDB) -q -x openocd-jtag.gdb $(TARGET_PREFIX_REAL).elf

# JTAG telnet
.PHONY: jt
jt:
	telnet localhost 4444

jr:
	./openocd_sess.py re

ju:
	./openocd_sess.py up

.PHONY: serial
serial:
	minicom -b 115200 -D $$(./tools/get_serial_dev.sh) -C uart_capture_$$(date +%S).bin

.PHONY: clean
clean:
	# Recursively find all objects and depends through
	# all directories
	find -name '*.o' -exec rm -v {} \;
	find -name '*.d' -exec rm -v {} \;
	# Only destroy what's seen in this dir
	find -maxdepth 1 -regex '.*.\(elf\|map\|sym\|img\)$$' -exec rm -v {} \;

DEPS := $(OBJS:.o=.d)

$(info $(DEPS))

%.d: %.c
	cpp -M $(INCLUDES_FLAGS) $< > $@

%.d: %.S
	cpp -M $(INCLUDES_FLAGS) $< > $@

%.d: %.cpp
	cpp -M $(INCLUDES_FLAGS) $< > $@

-include $(DEPS)

TARGETS := $(DEPS)\
	$(addprefix $(TARGET_PREFIX_REAL)., img sym elf)\
	$(addprefix $(TARGET_PREFIX_QEMU)., img sym elf)

$(info TARGETS: $(TARGETS))

.PHONY: all
all: $(TARGETS)
