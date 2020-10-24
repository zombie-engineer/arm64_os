set pri pre on
target extended-remote :3333
p $pc += 4
b usb_hcd_attach_root_hub
b usb_hcd_start
