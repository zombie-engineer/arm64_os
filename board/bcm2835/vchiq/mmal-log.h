#pragma once
#include <log.h>

extern int mmal_log_level;

#define MMAL_UNIT_TAG "mmal"
#define MMAL_INFO(__fmt, ...) \
  LOG(mmal_log_level, INFO, MMAL_UNIT_TAG, __fmt, ##__VA_ARGS__)

#define MMAL_ERR(__fmt, ...) \
  LOG(mmal_log_level, ERR, MMAL_UNIT_TAG, "%s:" __fmt __endline, __func__, ##__VA_ARGS__)

#define LOG(__log_filter, __log_level, __unit_tag, __fmt, ...) if (__log_filter >= LOG_LEVEL_ ## __log_level) \
  printf(__LOG_PREFIX(__unit_tag, __log_level) __fmt __endline, ##__VA_ARGS__)
