OBJS_ARMV8 = arch/armv8/mmu.o arch/armv8/armv8.o
$(info OBJS_ARMV8 $(OBJS_ARMV8))

arch/armv8/armv8.o: arch/armv8/armv8.S
	$(CC) $(CFLAGS) -c $? -o $@ 

