.PHONY: board/bcm2835/usb/dwc2_regs_bits.h
board/bcm2835/usb/dwc2_regs_bits.h: board/bcm2835/usb/dwc2_regs_spec.yaml $(REGTOOL)
	python3 $(REGTOOL) $^ > $@

.PHONY: board/bcm2835/emmc/emmc_regs_bits.h
board/bcm2835/emmc/emmc_regs_bits.h: board/bcm2835/emmc/emmc_regs_spec.yaml $(REGTOOL)
	python3 $(REGTOOL) $^ > $@


OBJS_BOARD_BCM2835 := bcm2835_arm_timer \
	usb/dwc2 \
	usb/dwc2_channel \
	usb/dwc2_transfer \
	usb/dwc2_xfer_control \
	usb/hcd \
	usb/hid_parser \
	usb/hcd_transfer \
	usb/hcd_mass_storage \
	usb/hcd_hid \
	usb/hcd_hub \
	usb/root_hub \
	usb/usb_xfer_queue \
	emmc/emmc \
	emmc/emmc_cmd \
	emmc/emmc_initialize \
	emmc/emmc_utils \
	bcm2835_systimer \
	bcm2835_gpio \
	bcm2835_gpio_access \
	i2c_master \
	bsc_slave

DIRPATH := board/bcm2835
INCLUDES += board/bcm2835/include

OBJS_BOARD_BCM2835 := $(addprefix $(DIRPATH)/, $(addsuffix .o, $(OBJS_BOARD_BCM2835)))
$(info $(OBJS_BOARD_BCM2835))
