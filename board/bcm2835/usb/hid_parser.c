#include "hid_parser.h"
#include <common.h>

#define HID_REPORT_ITEM_TYPE_MAIN   0
#define HID_REPORT_ITEM_TYPE_GLOBAL 1
#define HID_REPORT_ITEM_TYPE_LOCAL  2

#define HID_REPORT_ITEM_MAIN_INPUT          0b10000000
#define HID_REPORT_ITEM_MAIN_OUTPUT         0b10010000
#define HID_REPORT_ITEM_MAIN_FEATURE        0b10110000
#define HID_REPORT_ITEM_MAIN_COLLECTION     0b10100000
#define HID_REPORT_ITEM_MAIN_END_COLLECTION 0b11000000

#define HID_REPORT_ITEM_GLOBAL_USAGE_PAGE   0b00000100
#define HID_REPORT_ITEM_GLOBAL_USAGE_MIN    0b00010100
#define HID_REPORT_ITEM_GLOBAL_USAGE_MAX    0b00100100
#define HID_REPORT_ITEM_GLOBAL_PHYS_MIN     0b00110100
#define HID_REPORT_ITEM_GLOBAL_PHYS_MAX     0b01000100
#define HID_REPORT_ITEM_GLOBAL_UNIT_EXP     0b01010100
#define HID_REPORT_ITEM_GLOBAL_UNIT         0b01100100
#define HID_REPORT_ITEM_GLOBAL_REPORT_SIZE  0b01110100
#define HID_REPORT_ITEM_GLOBAL_REPORT_ID    0b10000100
#define HID_REPORT_ITEM_GLOBAL_REPORT_COUNT 0b10010100
#define HID_REPORT_ITEM_GLOBAL_PUSH         0b10100100
#define HID_REPORT_ITEM_GLOBAL_POP          0b10110100

#define HID_REPORT_ITEM_LOCAL_USAGE            0b00001000
#define HID_REPORT_ITEM_LOCAL_USAGE_MAX        0b00011000
#define HID_REPORT_ITEM_LOCAL_USAGE_MIN        0b00101000
#define HID_REPORT_ITEM_LOCAL_DESIGNATOR_INDEX 0b00111000
#define HID_REPORT_ITEM_LOCAL_DESIGNATOR_MIN   0b01001000
#define HID_REPORT_ITEM_LOCAL_DESIGNATOR_MAX   0b01011000
#define HID_REPORT_ITEM_LOCAL_STRING_INDEX     0b01111000
#define HID_REPORT_ITEM_LOCAL_STRING_MIN       0b10001000
#define HID_REPORT_ITEM_LOCAL_STRING_MAX       0b10011000
#define HID_REPORT_ITEM_LOCAL_DELIMITER        0b10101000

const char *hid_report_item_type_to_string(int type)
{
  switch(type) {
    case HID_REPORT_ITEM_TYPE_MAIN: return "MAIN";
    case HID_REPORT_ITEM_TYPE_GLOBAL: return "GLOBAL";
    case HID_REPORT_ITEM_TYPE_LOCAL: return "LOCAL";
    default: return "UNKNOWN";
  }
}

const char *hid_report_item_main_to_string(int v)
{
#define __CASE(__x) case HID_REPORT_ITEM_MAIN_ ## __x: return(#__x)
  switch (v) {
    __CASE(INPUT);
    __CASE(OUTPUT);
    __CASE(FEATURE);
    __CASE(COLLECTION);
    __CASE(END_COLLECTION);
    default: return "UNKNOWN";
  }
#undef __CASE
}

const char *hid_report_item_global_to_string(int v)
{
#define __CASE(__x) case HID_REPORT_ITEM_GLOBAL_ ## __x: return(#__x)
  switch (v) {
    __CASE(USAGE_PAGE);
    __CASE(USAGE_MIN);
    __CASE(USAGE_MAX);
    __CASE(PHYS_MIN);
    __CASE(PHYS_MAX);
    __CASE(UNIT_EXP);
    __CASE(UNIT);
    __CASE(REPORT_SIZE);
    __CASE(REPORT_ID);
    __CASE(REPORT_COUNT);
    __CASE(PUSH);
    __CASE(POP);
    default: return "UNKNOWN";
  }
#undef __CASE
}

const char *hid_report_item_local_to_string(int v)
{
#define __CASE(__x) case HID_REPORT_ITEM_LOCAL_ ## __x: return(#__x)
  switch (v) {
    __CASE(USAGE);
    __CASE(USAGE_MAX);
    __CASE(USAGE_MIN);
    __CASE(DESIGNATOR_INDEX);
    __CASE(DESIGNATOR_MIN);
    __CASE(DESIGNATOR_MAX);
    __CASE(STRING_INDEX);
    __CASE(STRING_MIN);
    __CASE(STRING_MAX);
    __CASE(DELIMITER);
    default: return "UNKNOWN";
}
#undef __CASE
}

#define PARSER_FETCH_CHAR(__to) \
  if (ptr < end) \
    __to = *ptr++;\
  else\
    goto err_out_of_range

void usb_hid_parse_report_descriptor(const void *buf, int bufsz)
{
  char data;
  char item_desc;
  char item_type;
  char item_size;
  char item_tag;
  const char *ptr = buf;
  const char *end = ptr + bufsz;
  while (1) {
    PARSER_FETCH_CHAR(item_desc);
    if (item_desc == 0b11111110) {
      /* long item */
      PARSER_FETCH_CHAR(item_size);
      PARSER_FETCH_CHAR(item_tag);
    } else {
      /* short item */
      item_size = (item_desc >> 0) & 3;
      item_type = (item_desc >> 2) & 3;
      item_tag  = (item_desc >> 4) & 0xf;
    }
    printf("ITEM: type:%s(%d), size:%d, tag:%d", hid_report_item_type_to_string(item_type), item_type, item_size, item_tag);
    switch(item_type) {
      case HID_REPORT_ITEM_TYPE_MAIN:
        printf(":%s", hid_report_item_main_to_string(item_desc & 0b11111100));
        break;
      case HID_REPORT_ITEM_TYPE_GLOBAL:
        printf(":%s", hid_report_item_global_to_string(item_desc & 0b11111100));
        break;
      case HID_REPORT_ITEM_TYPE_LOCAL:
        printf(":%s", hid_report_item_local_to_string(item_desc & 0b11111100));
        break;
    }
    for (; item_size; item_size--) {
      PARSER_FETCH_CHAR(data);
      printf(" %02x", data); 
    }
    
    puts("\r\n");
   // ptr += item_size;
  }
err_out_of_range:
  return;
}
