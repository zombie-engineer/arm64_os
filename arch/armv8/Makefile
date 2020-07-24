OBJS_ARMV8 = \
	armv8.o \
	armv8_cpuctx_routines.o \
	armv8_lock.o \
	armv8_cpuctx.o \
	armv8_printers.o \
	armv8_vector.o \
	armv8_timer.o \
	armv8_generic_timer.o \
	mmu.o \
	start.o \
	percpu_data.o

$(info OBJS_ARMV8 $(OBJS_ARMV8))

OBJS_ARMV8 := $(addprefix arch/armv8/, $(OBJS_ARMV8))

# arch/armv8/armv8.o: arch/armv8/armv8.S
#	$(CC) $(CFLAGS) -c $? -o $@

