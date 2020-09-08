#pragma once

/*
 * From: SCSI Commands Reference Manual
 */

#define SCSI_OPCODE_TEST_UNIT_READY   0x00
#define SCSI_OPCODE_REQUEST_SENSE     0x03
#define SCSI_OPCODE_INQUIRY           0x12
#define SCSI_OPCODE_READ_10           0x28
#define SCSI_OPCODE_WRITE_10          0x2a
#define SCSI_OPCODE_READ_CAPACITY_10  0x25
#define SCSI_OPCODE_LOG_SELECT        0x4c

struct scsi_op_test_unit_ready {
  uint8_t opcode          ;
  uint8_t reserved[4]     ; // +1
  uint8_t control         ; // +5
} PACKED;

struct scsi_op_request_sense {
  uint8_t opcode          ;
  uint8_t desc         : 1; // +1
  uint8_t reserved     : 7;
  uint8_t reserved_2[2]   ; // +2
  uint8_t alloc_length    ; // +4
  uint8_t control         ; // +5
} PACKED;

struct scsi_op_log_select {
  uint8_t opcode          ;
  uint8_t sp           : 1; // +1
  uint8_t pcr          : 1;
  uint8_t reserved0    : 6;
  uint8_t page_code    : 6; // +2
  uint8_t pc           : 2;
  uint8_t subpage_code    ; // +3
  uint8_t reserved[3]     ; // +4
  uint16_t param_list_len ; // +6
  uint8_t control         ; // +9
} PACKED;

struct scsi_op_read_10 {
  uint8_t opcode       ;
  uint8_t obsolete0 : 2; // +1
  uint8_t rarc      : 1;
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t rdprotect : 3;

  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_op_write_10 {
  uint8_t opcode       ;

  uint8_t obsolete0 : 2; // +1
  uint8_t reserved0 : 1; // +1
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t wrprotect : 3;
  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_op_read_capacity_10 {
  uint8_t opcode       ;
  uint8_t obsolete0 : 2; // +1
  uint8_t rarc      : 1;
  uint8_t fua       : 1;
  uint8_t dpo       : 1;
  uint8_t rdprotect : 3;

  uint32_t lba         ; // +2
  uint8_t group_num : 5; // +6
  uint8_t reserved  : 3;
  uint16_t transfer_len; // +7
  uint8_t control      ; // +9
} PACKED;

struct scsi_read_capacity_param {
  uint32_t logic_address;
  uint32_t block_length;
} PACKED;

struct scsi_op_inquiry {
  uint8_t opcode;
  uint8_t evpd : 1;
  uint8_t reserved1: 7;
  uint8_t page_code;
  uint16_t allocation_length;
  uint8_t control;
} PACKED;

struct scsi_response_inquiry {
  uint8_t peripheral_device_type : 5;
  uint8_t peripheral_qualifier   : 3;
  uint8_t reserved0              : 7; // +1
  uint8_t rmb                    : 1;
  uint8_t version                   ; // +2
  uint8_t response_data_format   : 4; // +3
  uint8_t hisup                  : 1;
  uint8_t normaca                : 1;
  uint8_t obsolete0              : 2;
  uint8_t additional_length         ; // +4 /* N-1 */
  uint8_t protect                : 1; // +5
  uint8_t obsolete1              : 2;
  uint8_t tpc                    : 1;
  uint8_t tpgs                   : 2;
  uint8_t acc                    : 1;
  uint8_t sccs                   : 1;

  uint8_t obsolete2              : 4; // +6
  uint8_t multip                 : 1;
  uint8_t vs                     : 1;
  uint8_t encserv                : 1;
  uint8_t obsolete3              : 1;

  uint8_t vs2                    : 1; // +7
  uint8_t cmdque                 : 1;
  uint8_t obsolete4              : 6;

  char    vendor_id[8]           ;   // +8
  char    product_id[16]         ;   // +16
  char    product_revision_lvl[4];   // +32
  char    drive_serial_number[8] ;   // +36
  char    vendor_unique[12]      ;   // +44
  char    reserved1              ;   // +56
  char    reserved2              ;   // +57
  uint16_t version_descriptor[8] ;   // +58
  char    reserved[22]           ;   // +74
  char    copyright_notice[0]    ;   // +96
} PACKED;

