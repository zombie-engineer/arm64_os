#pragma once

#define EMMC_BLOCK_SIZE 1024

int emmc_init(void);
void emmc_report(void);
int emmc_read(int blocknum, int numblocks, char *buf, int bufsz);
