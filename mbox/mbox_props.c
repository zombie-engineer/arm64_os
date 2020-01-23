#include <mbox/mbox_props.h>
#include <mbox/mbox.h>
#include <stringlib.h>


#define MBOX_MSG_T(message) mbox_msg_ ## message ## _t
#define MBOX_TAG_T(message) mbox_tag_ ## message ## _t
#define MBOX_REQ_T(message) mbox_req_ ## message ## _t
#define MBOX_RSP_T(message) mbox_rsp_ ## message ## _t

#define MBOX_PROP_CHECKED_CALL \
  if (!mbox_call(MBOX_CH_PROP)) \
    return -1

#define DECL_STRUCT_0(name)\
  typedef struct {\
  } __attribute__((packed)) name

#define DECL_STRUCT_1(name, fld1)\
  typedef struct {\
    uint32_t fld1;\
  } __attribute__((packed)) name

#define DECL_STRUCT_2(name, fld1, fld2)\
  typedef struct {\
    uint32_t fld1;\
    uint32_t fld2;\
  } __attribute__((packed)) name

#define DECL_STRUCT_3(name, fld1, fld2, fld3)\
  typedef struct {\
    uint32_t fld1;\
    uint32_t fld2;\
    uint32_t fld3;\
  } __attribute__((packed)) name


#define DECL_MBOX_REQ_0T(name)                   DECL_STRUCT_0(MBOX_REQ_T(name))
#define DECL_MBOX_REQ_1T(name, fld1)             DECL_STRUCT_1(MBOX_REQ_T(name), fld1)
#define DECL_MBOX_REQ_2T(name, fld1, fld2)       DECL_STRUCT_2(MBOX_REQ_T(name), fld1, fld2)
#define DECL_MBOX_REQ_3T(name, fld1, fld2, fld3) DECL_STRUCT_3(MBOX_REQ_T(name), fld1, fld2, fld3)

#define DECL_MBOX_RSP_0T(name)                   DECL_STRUCT_0(MBOX_RSP_T(name))
#define DECL_MBOX_RSP_1T(name, fld1)             DECL_STRUCT_1(MBOX_RSP_T(name), fld1)
#define DECL_MBOX_RSP_2T(name, fld1, fld2)       DECL_STRUCT_2(MBOX_RSP_T(name), fld1, fld2)
#define DECL_MBOX_RSP_3T(name, fld1, fld2, fld3) DECL_STRUCT_3(MBOX_RSP_T(name), fld1, fld2, fld3)


#define DECL_MBOX_TAG_T(tag) \
  typedef struct { \
    uint32_t id;\
    uint32_t req_len;\
    uint32_t rsp_len;\
    union {\
      MBOX_REQ_T(tag) req;\
      MBOX_RSP_T(tag) rsp;\
    } u;\
  } __attribute__((packed)) MBOX_TAG_T(tag);


#define DECL_MBOX_MSG_T(message) \
  typedef struct {\
    uint32_t msg_size;\
    uint32_t msg_type;\
    mbox_tag_ ## message ## _t tag;\
    uint32_t end_tag;\
  } __attribute__((packed)) MBOX_MSG_T(message)

#define DECL_MBOX_1TAG_MSG_T(message) \
  DECL_MBOX_TAG_T(message); \
  DECL_MBOX_MSG_T(message);


#define DECL_MBOX_MSG(message, tg) \
  volatile MBOX_MSG_T(message) *m = (MBOX_MSG_T(message)*)(mbox_buffer);\
  memset((void*)m, 0, sizeof(*m));\
  m->msg_size = sizeof(*m);\
  m->msg_type = MBOX_REQUEST;\
  m->tag.id = tg;\
  m->tag.req_len = sizeof(m->tag.u);\
  /*m->tag.rsp_len = sizeof(m->tag.u.rsp);*/\
  m->end_tag = MBOX_TAG_LAST;


#define MBOX_GET_RSP(fld) m->tag.u.rsp.fld


int mbox_get_firmware_rev()
{
  mbox_buffer[0] = 7 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_FIRMWARE_REV;
  mbox_buffer[3] = 4;
  mbox_buffer[4] = 4;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox_buffer[5];
  return -1;
}

DECL_MBOX_REQ_2T(get_mac_addr, mac_address_1, mac_address_2);
DECL_MBOX_RSP_2T(get_mac_addr, mac_address_1, mac_address_2);
DECL_MBOX_1TAG_MSG_T(get_mac_addr);

int mbox_get_mac_addr(char *mac_start, char *mac_end)
{
  int i;
  DECL_MBOX_MSG(get_mac_addr, MBOX_TAG_GET_MAC_ADDR);
  if (mbox_call(MBOX_CH_PROP)) {
    for (i = 0; i < 6; ++i)
    {
      if (mac_start + i >= mac_end)
        break;
      mac_start[i] = ((char*)(&mbox_buffer[5]))[i];
    }
    return 0;
  }
  return -1;
}

int mbox_get_board_model()
{
  mbox_buffer[0] = 7 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_BOARD_MODEL;
  mbox_buffer[3] = 4;
  mbox_buffer[4] = 4;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox_buffer[5];
  return -1;
}

int mbox_get_board_rev()
{
  mbox_buffer[0] = 7 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_BOARD_REV;
  mbox_buffer[3] = 4;
  mbox_buffer[4] = 4;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
    return mbox_buffer[5];
  return -1;
}

unsigned long mbox_get_board_serial()
{
  unsigned long res;

  mbox_buffer[0] = 8 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_BOARD_SERIAL;
  mbox_buffer[3] = 8;
  mbox_buffer[4] = 8;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = 0;
  mbox_buffer[7] = MBOX_TAG_LAST;
  res = mbox_buffer[5];
  res <<= 32;
  res |= mbox_buffer[6];
  return res;
}

int mbox_get_arm_memory(int *base_addr, int *byte_size)
{
  if (base_addr == 0 || byte_size == 0)
    return -1;

  mbox_buffer[0] = 8 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_ARM_MEMORY;
  mbox_buffer[3] = 8;
  mbox_buffer[4] = 8;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = 0;
  mbox_buffer[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
  {
    *base_addr = mbox_buffer[5];
    *byte_size = mbox_buffer[6];
    return 0;
  }
  return -1;
}

int mbox_get_vc_memory(int *base_addr, int *byte_size)
{
  if (base_addr == 0 || byte_size == 0)
    return -1;

  mbox_buffer[0] = 8 * 4;
  mbox_buffer[1] = MBOX_REQUEST;
  mbox_buffer[2] = MBOX_TAG_GET_VC_MEMORY;
  mbox_buffer[3] = 8;
  mbox_buffer[4] = 8;
  mbox_buffer[5] = 0;
  mbox_buffer[6] = 0;
  mbox_buffer[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP))
  {
    *base_addr = mbox_buffer[5];
    *byte_size = mbox_buffer[6];
    return 0;
  }
  return -1;
}

DECL_MBOX_REQ_2T(get_clock_state, clock_id, padding);
DECL_MBOX_RSP_2T(get_clock_state, enabled, exists);
DECL_MBOX_1TAG_MSG_T(get_clock_state);

int mbox_get_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists)
{
  DECL_MBOX_MSG(get_clock_state, MBOX_TAG_GET_CLOCK_STATE);
  m->tag.u.req.clock_id = clock_id;
  if (mbox_call(MBOX_CH_PROP)) {
    *enabled = MBOX_GET_RSP(enabled);
    *exists = MBOX_GET_RSP(exists);
    return 0;
  }
  return -1;
}

DECL_MBOX_REQ_1T(set_clock_state, clock_id);
DECL_MBOX_RSP_2T(set_clock_state, enabled, exists);
DECL_MBOX_1TAG_MSG_T(set_clock_state);

int mbox_set_clock_state(uint32_t clock_id, uint32_t* enabled, uint32_t *exists)
{
  DECL_MBOX_MSG(set_clock_state, MBOX_TAG_SET_CLOCK_STATE);
  m->tag.u.req.clock_id = clock_id;
  if (mbox_call(MBOX_CH_PROP)) {
    *enabled = MBOX_GET_RSP(enabled);
    *exists = MBOX_GET_RSP(exists);
    return 0;
  }
  return -1;
}

DECL_MBOX_REQ_2T(get_clock_rate, clock_id, paddding);
DECL_MBOX_RSP_2T(get_clock_rate, clock_id, rate_hz);
DECL_MBOX_1TAG_MSG_T(get_clock_rate);

#define GET_CLOCK_RATE(tg) \
  DECL_MBOX_MSG(get_clock_rate, tg); \
  m->tag.u.req.clock_id = clock_id;\
  if (mbox_call(MBOX_CH_PROP) && MBOX_GET_RSP(clock_id) == clock_id) {\
    *clock_rate = MBOX_GET_RSP(rate_hz);\
    return 0;\
  }\
  return -1

int mbox_get_clock_rate(uint32_t clock_id, uint32_t *clock_rate)
{
  GET_CLOCK_RATE(MBOX_TAG_GET_CLOCK_RATE);
}

int mbox_get_min_clock_rate(uint32_t clock_id, uint32_t *clock_rate)
{
  GET_CLOCK_RATE(MBOX_TAG_GET_MIN_CLOCK_RATE);
}

int mbox_get_max_clock_rate(uint32_t clock_id, uint32_t *clock_rate)
{
  GET_CLOCK_RATE(MBOX_TAG_GET_MAX_CLOCK_RATE);
}


DECL_MBOX_REQ_3T(set_clock_rate, clock_id, rate_hz, skip_turbo);
DECL_MBOX_RSP_2T(set_clock_rate, clock_id, rate_hz);
DECL_MBOX_1TAG_MSG_T(set_clock_rate);

int mbox_set_clock_rate(uint32_t clock_id, uint32_t *clock_rate, uint32_t skip_turbo)
{
  DECL_MBOX_MSG(set_clock_rate, MBOX_TAG_SET_CLOCK_RATE);
  m->tag.u.req.clock_id = clock_id;
  m->tag.u.req.rate_hz = *clock_rate;
  m->tag.u.req.skip_turbo = skip_turbo;
  if (mbox_call(MBOX_CH_PROP) && MBOX_GET_RSP(clock_id) == clock_id) {
    *clock_rate = MBOX_GET_RSP(rate_hz);
    return 0;
  }
  return -1;
}

DECL_MBOX_REQ_0T(get_virt_wh);
DECL_MBOX_RSP_2T(get_virt_wh, width, height);
DECL_MBOX_1TAG_MSG_T(get_virt_wh);

int mbox_get_virt_wh(uint32_t *out_width, uint32_t *out_height)
{
  DECL_MBOX_MSG(get_virt_wh, MBOX_TAG_GET_VIRT_WIDTH_HEIGHT);
  MBOX_PROP_CHECKED_CALL;

  *out_width = MBOX_GET_RSP(width);
  *out_height = MBOX_GET_RSP(height);
  return 0;
}
