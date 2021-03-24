#pragma once

#define CHECK_ERR(__fmt, ...) \
  if (err != ERR_OK) { \
    MMAL_ERR("err: %d, " __fmt, err, ##__VA_ARGS__); \
    goto out_err; \
  }

#define CHECK_ERR_PTR(__ptr, __fmt, ...) \
  if (IS_ERR(__ptr)) { \
    err = PTR_ERR(__ptr);\
    MMAL_ERR("err: %d, " __fmt, err, ##__VA_ARGS__); \
    goto out_err; \
  }
