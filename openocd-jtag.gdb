set pri pre on
target extended-remote :3333
p $pc += 4
b vchiq_handmade
# b vchiq_mmal_port_set_format
b vchiq_camera_run_preview
b vchiq_prepare_fragments

source gdb.helpers
