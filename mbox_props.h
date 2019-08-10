#pragma once

int mbox_get_firmware_rev();

int mbox_get_board_model();

int mbox_get_board_rev();

unsigned long mbox_get_board_serial();

int mbox_get_arm_memory(int *base_addr, int *byte_size);

int mbox_get_vc_memory(int *base_addr, int *byte_size);

int mbox_get_mac_addr(char *mac_start, char *mac_end);
