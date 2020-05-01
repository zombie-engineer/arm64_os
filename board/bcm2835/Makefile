board/bcm2835/usb/registers_bits.h: board/bcm2835/usb/registers.yaml
	python3 tools/gen_reg_headers.py $^ > $?

OBJS_BOARD_BCM2835 := bcm2835_arm_timer \
	usb/bcm2835_usb \
	usb/root_hub \
	bcm2835_systimer \
	i2c_master \
	bsc_slave

DIRPATH := board/bcm2835
INCLUDES += board/bcm2835/include

OBJS_BOARD_BCM2835 := $(addprefix $(DIRPATH)/, $(addsuffix .o, $(OBJS_BOARD_BCM2835)))
$(info $(OBJS_BOARD_BCM2835))
