#pragma once
#include <config.h>

void fs_probe_early(void);
#if defined(ENABLE_JTAG_DOWNLOAD) || defined(ENABLE_UART_DOWNLOAD)
int write_image_to_sd(const char *filename, char *image_start, char *image_end);
#endif
