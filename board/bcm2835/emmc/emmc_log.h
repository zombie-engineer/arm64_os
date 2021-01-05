#pragma once

#include <log.h>

extern int emmc_log_level;

#define EMMC_LOG(__fmt, ...) LOG(emmc_log_level, INFO, "emmc", __fmt, ##__VA_ARGS__)
#define EMMC_DBG(__fmt, ...) LOG(emmc_log_level, DEBUG, "emmc", __fmt, ##__VA_ARGS__)
#define EMMC_DBG2(__fmt, ...) LOG(emmc_log_level, DEBUG2, "emmc", __fmt, ##__VA_ARGS__)
#define EMMC_WARN(__fmt, ...) LOG(emmc_log_level, WARN, "emmc", __fmt, ##__VA_ARGS__)
#define EMMC_CRIT(__fmt, ...) LOG(emmc_log_level, CRIT, "emmc", __fmt, ##__VA_ARGS__)
#define EMMC_ERR(__fmt, ...) LOG(emmc_log_level, ERR, "emmc", __fmt, ##__VA_ARGS__)

