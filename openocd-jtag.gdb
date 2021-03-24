set pri pre on
target extended-remote :3333
p $pc += 4
b vchiq_handmade
b vchiq_handmade_connect
b vchiq_irq_handler
b vchiq_camera_run

source gdb.helpers
