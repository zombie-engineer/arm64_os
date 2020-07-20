#pragma once
#include <common.h>

#define LOG_LEVEL_NONE   0
#define LOG_LEVEL_CRIT   1
#define LOG_LEVEL_ERR    2
#define LOG_LEVEL_WARN   3
#define LOG_LEVEL_INFO   4
#define LOG_LEVEL_DEBUG  5
#define LOG_LEVEL_DEBUG2 6
#define LOG_LEVEL_DEBUG3 7

/*
 * "[ UNIT_TAG LOG_LEVEL] "
 */
#define __LOG_PREFIX(__unit_tag, __log_level) "["__unit_tag" " #__log_level "] "

/*
 * "[ UNIT_TAG LOG_LEVEL] __fmt", args
 */
#define LOG(__log_filter, __log_level, __unit_tag, __fmt, ...) if (__log_filter >= LOG_LEVEL_ ## __log_level) \
  printf(__LOG_PREFIX(__unit_tag, __log_level) __fmt __endline, ##__VA_ARGS__)
