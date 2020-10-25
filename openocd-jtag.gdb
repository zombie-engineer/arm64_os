set pri pre on
target extended-remote :3333
p $pc += 4
b usb_mass_inquiry
b usb_hcd_start
