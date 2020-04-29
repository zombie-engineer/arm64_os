board/bcm2835/bcm2835_usb_registers_bits.h: board/bcm2835/bcm2835_usb_registers.yaml
	python3 tools/gen_reg_headers.py board/bcm2835/bcm2835_usb_registers.yaml > board/bcm2835/bcm2835_usb_registers_bits.h

OBJS_BOARD_BCM2835 := bcm2835_arm_timer \
	bcm2835_usb \
	bcm2835_systimer \
	i2c_master \
	bsc_slave

DIRPATH := board/bcm2835

OBJS_BOARD_BCM2835 := $(addprefix $(DIRPATH)/, $(addsuffix .o, $(OBJS_BOARD_BCM2835)))
$(info $(OBJS_BOARD_BCM2835))
