source kernel-helpers.gdb
set pri pre on
target extended-remote :3333
p $pc += 4
# b vchiq_tx_header_to_slot_idx
# b vchiq_process_free_queue
# b vchiq_handmade
# b vchiq_handmade_connect
# b vchiq_irq_handler
# b vchiq_camera_run
# b mmal_buffer_to_host_cb
# b mmal_port_buffer_io_work
# b vcanvas_init
# b mems_service_data_callback
# b tft_lcd_init
# b tft_cmd
# b vchiq_event_signal
# b vchiq_parse_rx
# b schedule_handle_flag_waiting
c

source gdb.helpers
