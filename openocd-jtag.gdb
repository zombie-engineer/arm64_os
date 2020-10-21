target extended-remote :3333
p $pc += 4
b usb_hcd_enumerate_device
b usb_hub_power_on_ports
