#pragma once

#define EMMC_BLOCK_SIZE 1024

#define EMMC_WAIT_TIMEOUT_USEC 1000000

int emmc_init(void);
void emmc_report(void);
int emmc_read(int blocknum, int numblocks, char *buf, int bufsz);
int emmc_write(int blocknum, int numblocks, char *buf, int bufsz);
