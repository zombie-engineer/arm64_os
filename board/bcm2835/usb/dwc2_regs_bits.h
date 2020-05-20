#pragma once

#define USB_HOST_CHAR_GET_MAX_PACK_SZ(v)         BF_EXTRACT(v, 0 , 11)
#define USB_HOST_CHAR_GET_EP(v)                  BF_EXTRACT(v, 11, 4 )
#define USB_HOST_CHAR_GET_EP_DIR(v)              BF_EXTRACT(v, 15, 1 )
#define USB_HOST_CHAR_GET_IS_LOW(v)              BF_EXTRACT(v, 17, 1 )
#define USB_HOST_CHAR_GET_EP_TYPE(v)             BF_EXTRACT(v, 18, 2 )
#define USB_HOST_CHAR_GET_PACK_PER_FRM(v)        BF_EXTRACT(v, 20, 2 )
#define USB_HOST_CHAR_GET_DEV_ADDR(v)            BF_EXTRACT(v, 22, 7 )
#define USB_HOST_CHAR_GET_ODD_FRAME(v)           BF_EXTRACT(v, 29, 1 )
#define USB_HOST_CHAR_GET_CHAN_DISABLE(v)        BF_EXTRACT(v, 30, 1 )
#define USB_HOST_CHAR_GET_CHAN_ENABLE(v)         BF_EXTRACT(v, 31, 1 )
#define USB_HOST_CHAR_CLR_SET_MAX_PACK_SZ(v, set)        BF_CLEAR_AND_SET(v, set, 0 , 11)
#define USB_HOST_CHAR_CLR_SET_EP(v, set)                 BF_CLEAR_AND_SET(v, set, 11, 4 )
#define USB_HOST_CHAR_CLR_SET_EP_DIR(v, set)             BF_CLEAR_AND_SET(v, set, 15, 1 )
#define USB_HOST_CHAR_CLR_SET_IS_LOW(v, set)             BF_CLEAR_AND_SET(v, set, 17, 1 )
#define USB_HOST_CHAR_CLR_SET_EP_TYPE(v, set)            BF_CLEAR_AND_SET(v, set, 18, 2 )
#define USB_HOST_CHAR_CLR_SET_PACK_PER_FRM(v, set)       BF_CLEAR_AND_SET(v, set, 20, 2 )
#define USB_HOST_CHAR_CLR_SET_DEV_ADDR(v, set)           BF_CLEAR_AND_SET(v, set, 22, 7 )
#define USB_HOST_CHAR_CLR_SET_ODD_FRAME(v, set)          BF_CLEAR_AND_SET(v, set, 29, 1 )
#define USB_HOST_CHAR_CLR_SET_CHAN_DISABLE(v, set)       BF_CLEAR_AND_SET(v, set, 30, 1 )
#define USB_HOST_CHAR_CLR_SET_CHAN_ENABLE(v, set)        BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_HOST_CHAR_CLR_MAX_PACK_SZ(v)         BF_CLEAR(v, 0 , 11)
#define USB_HOST_CHAR_CLR_EP(v)                  BF_CLEAR(v, 11, 4 )
#define USB_HOST_CHAR_CLR_EP_DIR(v)              BF_CLEAR(v, 15, 1 )
#define USB_HOST_CHAR_CLR_IS_LOW(v)              BF_CLEAR(v, 17, 1 )
#define USB_HOST_CHAR_CLR_EP_TYPE(v)             BF_CLEAR(v, 18, 2 )
#define USB_HOST_CHAR_CLR_PACK_PER_FRM(v)        BF_CLEAR(v, 20, 2 )
#define USB_HOST_CHAR_CLR_DEV_ADDR(v)            BF_CLEAR(v, 22, 7 )
#define USB_HOST_CHAR_CLR_ODD_FRAME(v)           BF_CLEAR(v, 29, 1 )
#define USB_HOST_CHAR_CLR_CHAN_DISABLE(v)        BF_CLEAR(v, 30, 1 )
#define USB_HOST_CHAR_CLR_CHAN_ENABLE(v)         BF_CLEAR(v, 31, 1 )


static inline int usb_host_char_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,MAX_PACK_SZ:%x,EP:%x,EP_DIR:%x,IS_LOW:%x,EP_TYPE:%x,PACK_PER_FRM:%x,DEV_ADDR:%x,ODD_FRAME:%x,CHAN_DISABLE:%x,CHAN_ENABLE:%x",
    v,
    (int)USB_HOST_CHAR_GET_MAX_PACK_SZ(v),
    (int)USB_HOST_CHAR_GET_EP(v),
    (int)USB_HOST_CHAR_GET_EP_DIR(v),
    (int)USB_HOST_CHAR_GET_IS_LOW(v),
    (int)USB_HOST_CHAR_GET_EP_TYPE(v),
    (int)USB_HOST_CHAR_GET_PACK_PER_FRM(v),
    (int)USB_HOST_CHAR_GET_DEV_ADDR(v),
    (int)USB_HOST_CHAR_GET_ODD_FRAME(v),
    (int)USB_HOST_CHAR_GET_CHAN_DISABLE(v),
    (int)USB_HOST_CHAR_GET_CHAN_ENABLE(v));
}
#define USB_HOST_SPLT_GET_PORT_ADDR(v)           BF_EXTRACT(v, 0 , 7 )
#define USB_HOST_SPLT_GET_HUB_ADDR(v)            BF_EXTRACT(v, 7 , 7 )
#define USB_HOST_SPLT_GET_TRANS_POS(v)           BF_EXTRACT(v, 14, 2 )
#define USB_HOST_SPLT_GET_COMPLETE_SPLIT(v)      BF_EXTRACT(v, 16, 1 )
#define USB_HOST_SPLT_GET_SPLT_ENABLE(v)         BF_EXTRACT(v, 31, 1 )
#define USB_HOST_SPLT_CLR_SET_PORT_ADDR(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 7 )
#define USB_HOST_SPLT_CLR_SET_HUB_ADDR(v, set)           BF_CLEAR_AND_SET(v, set, 7 , 7 )
#define USB_HOST_SPLT_CLR_SET_TRANS_POS(v, set)          BF_CLEAR_AND_SET(v, set, 14, 2 )
#define USB_HOST_SPLT_CLR_SET_COMPLETE_SPLIT(v, set)     BF_CLEAR_AND_SET(v, set, 16, 1 )
#define USB_HOST_SPLT_CLR_SET_SPLT_ENABLE(v, set)        BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_HOST_SPLT_CLR_PORT_ADDR(v)           BF_CLEAR(v, 0 , 7 )
#define USB_HOST_SPLT_CLR_HUB_ADDR(v)            BF_CLEAR(v, 7 , 7 )
#define USB_HOST_SPLT_CLR_TRANS_POS(v)           BF_CLEAR(v, 14, 2 )
#define USB_HOST_SPLT_CLR_COMPLETE_SPLIT(v)      BF_CLEAR(v, 16, 1 )
#define USB_HOST_SPLT_CLR_SPLT_ENABLE(v)         BF_CLEAR(v, 31, 1 )


static inline int usb_host_splt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,PORT_ADDR:%x,HUB_ADDR:%x,TRANS_POS:%x,COMPLETE_SPLIT:%x,SPLT_ENABLE:%x",
    v,
    (int)USB_HOST_SPLT_GET_PORT_ADDR(v),
    (int)USB_HOST_SPLT_GET_HUB_ADDR(v),
    (int)USB_HOST_SPLT_GET_TRANS_POS(v),
    (int)USB_HOST_SPLT_GET_COMPLETE_SPLIT(v),
    (int)USB_HOST_SPLT_GET_SPLT_ENABLE(v));
}
#define USB_HOST_SIZE_GET_SIZE(v)                BF_EXTRACT(v, 0 , 19)
#define USB_HOST_SIZE_GET_PACKET_COUNT(v)        BF_EXTRACT(v, 19, 10)
#define USB_HOST_SIZE_GET_PID(v)                 BF_EXTRACT(v, 29, 2 )
#define USB_HOST_SIZE_GET_DO_PING(v)             BF_EXTRACT(v, 31, 1 )
#define USB_HOST_SIZE_CLR_SET_SIZE(v, set)               BF_CLEAR_AND_SET(v, set, 0 , 19)
#define USB_HOST_SIZE_CLR_SET_PACKET_COUNT(v, set)       BF_CLEAR_AND_SET(v, set, 19, 10)
#define USB_HOST_SIZE_CLR_SET_PID(v, set)                BF_CLEAR_AND_SET(v, set, 29, 2 )
#define USB_HOST_SIZE_CLR_SET_DO_PING(v, set)            BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_HOST_SIZE_CLR_SIZE(v)                BF_CLEAR(v, 0 , 19)
#define USB_HOST_SIZE_CLR_PACKET_COUNT(v)        BF_CLEAR(v, 19, 10)
#define USB_HOST_SIZE_CLR_PID(v)                 BF_CLEAR(v, 29, 2 )
#define USB_HOST_SIZE_CLR_DO_PING(v)             BF_CLEAR(v, 31, 1 )


static inline int usb_host_size_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,SIZE:%x,PACKET_COUNT:%x,PID:%x,DO_PING:%x",
    v,
    (int)USB_HOST_SIZE_GET_SIZE(v),
    (int)USB_HOST_SIZE_GET_PACKET_COUNT(v),
    (int)USB_HOST_SIZE_GET_PID(v),
    (int)USB_HOST_SIZE_GET_DO_PING(v));
}
#define USB_HOST_INTR_GET_XFER_COMPLETE(v)       BF_EXTRACT(v, 0 , 1 )
#define USB_HOST_INTR_GET_HALT(v)                BF_EXTRACT(v, 1 , 1 )
#define USB_HOST_INTR_GET_AHB_ERR(v)             BF_EXTRACT(v, 2 , 1 )
#define USB_HOST_INTR_GET_STALL(v)               BF_EXTRACT(v, 3 , 1 )
#define USB_HOST_INTR_GET_NAK(v)                 BF_EXTRACT(v, 4 , 1 )
#define USB_HOST_INTR_GET_ACK(v)                 BF_EXTRACT(v, 5 , 1 )
#define USB_HOST_INTR_GET_NYET(v)                BF_EXTRACT(v, 6 , 1 )
#define USB_HOST_INTR_GET_TRNSERR(v)             BF_EXTRACT(v, 7 , 1 )
#define USB_HOST_INTR_GET_BABBLERR(v)            BF_EXTRACT(v, 8 , 1 )
#define USB_HOST_INTR_GET_FRMOVRN(v)             BF_EXTRACT(v, 9 , 1 )
#define USB_HOST_INTR_GET_DATTGGLERR(v)          BF_EXTRACT(v, 10, 1 )
#define USB_HOST_INTR_GET_BUFNOTAVAIL(v)         BF_EXTRACT(v, 11, 1 )
#define USB_HOST_INTR_GET_EXCESSXMIT(v)          BF_EXTRACT(v, 12, 1 )
#define USB_HOST_INTR_GET_FRMLISTROLL(v)         BF_EXTRACT(v, 13, 1 )
#define USB_HOST_INTR_CLR_SET_XFER_COMPLETE(v, set)      BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define USB_HOST_INTR_CLR_SET_HALT(v, set)               BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define USB_HOST_INTR_CLR_SET_AHB_ERR(v, set)            BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define USB_HOST_INTR_CLR_SET_STALL(v, set)              BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define USB_HOST_INTR_CLR_SET_NAK(v, set)                BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define USB_HOST_INTR_CLR_SET_ACK(v, set)                BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_HOST_INTR_CLR_SET_NYET(v, set)               BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define USB_HOST_INTR_CLR_SET_TRNSERR(v, set)            BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define USB_HOST_INTR_CLR_SET_BABBLERR(v, set)           BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define USB_HOST_INTR_CLR_SET_FRMOVRN(v, set)            BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define USB_HOST_INTR_CLR_SET_DATTGGLERR(v, set)         BF_CLEAR_AND_SET(v, set, 10, 1 )
#define USB_HOST_INTR_CLR_SET_BUFNOTAVAIL(v, set)        BF_CLEAR_AND_SET(v, set, 11, 1 )
#define USB_HOST_INTR_CLR_SET_EXCESSXMIT(v, set)         BF_CLEAR_AND_SET(v, set, 12, 1 )
#define USB_HOST_INTR_CLR_SET_FRMLISTROLL(v, set)        BF_CLEAR_AND_SET(v, set, 13, 1 )
#define USB_HOST_INTR_CLR_XFER_COMPLETE(v)       BF_CLEAR(v, 0 , 1 )
#define USB_HOST_INTR_CLR_HALT(v)                BF_CLEAR(v, 1 , 1 )
#define USB_HOST_INTR_CLR_AHB_ERR(v)             BF_CLEAR(v, 2 , 1 )
#define USB_HOST_INTR_CLR_STALL(v)               BF_CLEAR(v, 3 , 1 )
#define USB_HOST_INTR_CLR_NAK(v)                 BF_CLEAR(v, 4 , 1 )
#define USB_HOST_INTR_CLR_ACK(v)                 BF_CLEAR(v, 5 , 1 )
#define USB_HOST_INTR_CLR_NYET(v)                BF_CLEAR(v, 6 , 1 )
#define USB_HOST_INTR_CLR_TRNSERR(v)             BF_CLEAR(v, 7 , 1 )
#define USB_HOST_INTR_CLR_BABBLERR(v)            BF_CLEAR(v, 8 , 1 )
#define USB_HOST_INTR_CLR_FRMOVRN(v)             BF_CLEAR(v, 9 , 1 )
#define USB_HOST_INTR_CLR_DATTGGLERR(v)          BF_CLEAR(v, 10, 1 )
#define USB_HOST_INTR_CLR_BUFNOTAVAIL(v)         BF_CLEAR(v, 11, 1 )
#define USB_HOST_INTR_CLR_EXCESSXMIT(v)          BF_CLEAR(v, 12, 1 )
#define USB_HOST_INTR_CLR_FRMLISTROLL(v)         BF_CLEAR(v, 13, 1 )


static inline int usb_host_intr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,XFER_COMPLETE:%x,HALT:%x,AHB_ERR:%x,STALL:%x,NAK:%x,ACK:%x,NYET:%x,TRNSERR:%x,BABBLERR:%x,FRMOVRN:%x,DATTGGLERR:%x,BUFNOTAVAIL:%x,EXCESSXMIT:%x,FRMLISTROLL:%x",
    v,
    (int)USB_HOST_INTR_GET_XFER_COMPLETE(v),
    (int)USB_HOST_INTR_GET_HALT(v),
    (int)USB_HOST_INTR_GET_AHB_ERR(v),
    (int)USB_HOST_INTR_GET_STALL(v),
    (int)USB_HOST_INTR_GET_NAK(v),
    (int)USB_HOST_INTR_GET_ACK(v),
    (int)USB_HOST_INTR_GET_NYET(v),
    (int)USB_HOST_INTR_GET_TRNSERR(v),
    (int)USB_HOST_INTR_GET_BABBLERR(v),
    (int)USB_HOST_INTR_GET_FRMOVRN(v),
    (int)USB_HOST_INTR_GET_DATTGGLERR(v),
    (int)USB_HOST_INTR_GET_BUFNOTAVAIL(v),
    (int)USB_HOST_INTR_GET_EXCESSXMIT(v),
    (int)USB_HOST_INTR_GET_FRMLISTROLL(v));
}
#define USB_GHWCFG2_GET_MODE(v)                  BF_EXTRACT(v, 0 , 3 )
#define USB_GHWCFG2_GET_ARCHITECTURE(v)          BF_EXTRACT(v, 3 , 2 )
#define USB_GHWCFG2_GET_SINGLE_POINT(v)          BF_EXTRACT(v, 5 , 1 )
#define USB_GHWCFG2_GET_HSPHY_INTERFACE(v)       BF_EXTRACT(v, 6 , 2 )
#define USB_GHWCFG2_GET_FSPHY_INTERFACE(v)       BF_EXTRACT(v, 8 , 2 )
#define USB_GHWCFG2_GET_NUM_EPS(v)               BF_EXTRACT(v, 10, 4 )
#define USB_GHWCFG2_GET_NUM_HOST_CHAN(v)         BF_EXTRACT(v, 14, 4 )
#define USB_GHWCFG2_GET_EN_PERIO_HOST(v)         BF_EXTRACT(v, 18, 1 )
#define USB_GHWCFG2_GET_DFIFO_DYNAMIC(v)         BF_EXTRACT(v, 19, 1 )
#define USB_GHWCFG2_GET_UNKNOWN4(v)              BF_EXTRACT(v, 20, 1 )
#define USB_GHWCFG2_GET_NPERIO_TX_QUEUE_DEPTH(v) BF_EXTRACT(v, 22, 2 )
#define USB_GHWCFG2_GET_PERIO_TX_QUEUE_DEPTH(v)  BF_EXTRACT(v, 24, 2 )
#define USB_GHWCFG2_GET_TOKEN_QUEUE_DEPTH(v)     BF_EXTRACT(v, 26, 5 )
#define USB_GHWCFG2_CLR_SET_MODE(v, set)                 BF_CLEAR_AND_SET(v, set, 0 , 3 )
#define USB_GHWCFG2_CLR_SET_ARCHITECTURE(v, set)         BF_CLEAR_AND_SET(v, set, 3 , 2 )
#define USB_GHWCFG2_CLR_SET_SINGLE_POINT(v, set)         BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_GHWCFG2_CLR_SET_HSPHY_INTERFACE(v, set)      BF_CLEAR_AND_SET(v, set, 6 , 2 )
#define USB_GHWCFG2_CLR_SET_FSPHY_INTERFACE(v, set)      BF_CLEAR_AND_SET(v, set, 8 , 2 )
#define USB_GHWCFG2_CLR_SET_NUM_EPS(v, set)              BF_CLEAR_AND_SET(v, set, 10, 4 )
#define USB_GHWCFG2_CLR_SET_NUM_HOST_CHAN(v, set)        BF_CLEAR_AND_SET(v, set, 14, 4 )
#define USB_GHWCFG2_CLR_SET_EN_PERIO_HOST(v, set)        BF_CLEAR_AND_SET(v, set, 18, 1 )
#define USB_GHWCFG2_CLR_SET_DFIFO_DYNAMIC(v, set)        BF_CLEAR_AND_SET(v, set, 19, 1 )
#define USB_GHWCFG2_CLR_SET_UNKNOWN4(v, set)             BF_CLEAR_AND_SET(v, set, 20, 1 )
#define USB_GHWCFG2_CLR_SET_NPERIO_TX_QUEUE_DEPTH(v, set) BF_CLEAR_AND_SET(v, set, 22, 2 )
#define USB_GHWCFG2_CLR_SET_PERIO_TX_QUEUE_DEPTH(v, set) BF_CLEAR_AND_SET(v, set, 24, 2 )
#define USB_GHWCFG2_CLR_SET_TOKEN_QUEUE_DEPTH(v, set)    BF_CLEAR_AND_SET(v, set, 26, 5 )
#define USB_GHWCFG2_CLR_MODE(v)                  BF_CLEAR(v, 0 , 3 )
#define USB_GHWCFG2_CLR_ARCHITECTURE(v)          BF_CLEAR(v, 3 , 2 )
#define USB_GHWCFG2_CLR_SINGLE_POINT(v)          BF_CLEAR(v, 5 , 1 )
#define USB_GHWCFG2_CLR_HSPHY_INTERFACE(v)       BF_CLEAR(v, 6 , 2 )
#define USB_GHWCFG2_CLR_FSPHY_INTERFACE(v)       BF_CLEAR(v, 8 , 2 )
#define USB_GHWCFG2_CLR_NUM_EPS(v)               BF_CLEAR(v, 10, 4 )
#define USB_GHWCFG2_CLR_NUM_HOST_CHAN(v)         BF_CLEAR(v, 14, 4 )
#define USB_GHWCFG2_CLR_EN_PERIO_HOST(v)         BF_CLEAR(v, 18, 1 )
#define USB_GHWCFG2_CLR_DFIFO_DYNAMIC(v)         BF_CLEAR(v, 19, 1 )
#define USB_GHWCFG2_CLR_UNKNOWN4(v)              BF_CLEAR(v, 20, 1 )
#define USB_GHWCFG2_CLR_NPERIO_TX_QUEUE_DEPTH(v) BF_CLEAR(v, 22, 2 )
#define USB_GHWCFG2_CLR_PERIO_TX_QUEUE_DEPTH(v)  BF_CLEAR(v, 24, 2 )
#define USB_GHWCFG2_CLR_TOKEN_QUEUE_DEPTH(v)     BF_CLEAR(v, 26, 5 )


static inline int usb_ghwcfg2_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,MODE:%x,ARCHITECTURE:%x,SINGLE_POINT:%x,HSPHY_INTERFACE:%x,FSPHY_INTERFACE:%x,NUM_EPS:%x,NUM_HOST_CHAN:%x,EN_PERIO_HOST:%x,DFIFO_DYNAMIC:%x,UNKNOWN4:%x,NPERIO_TX_QUEUE_DEPTH:%x,PERIO_TX_QUEUE_DEPTH:%x,TOKEN_QUEUE_DEPTH:%x",
    v,
    (int)USB_GHWCFG2_GET_MODE(v),
    (int)USB_GHWCFG2_GET_ARCHITECTURE(v),
    (int)USB_GHWCFG2_GET_SINGLE_POINT(v),
    (int)USB_GHWCFG2_GET_HSPHY_INTERFACE(v),
    (int)USB_GHWCFG2_GET_FSPHY_INTERFACE(v),
    (int)USB_GHWCFG2_GET_NUM_EPS(v),
    (int)USB_GHWCFG2_GET_NUM_HOST_CHAN(v),
    (int)USB_GHWCFG2_GET_EN_PERIO_HOST(v),
    (int)USB_GHWCFG2_GET_DFIFO_DYNAMIC(v),
    (int)USB_GHWCFG2_GET_UNKNOWN4(v),
    (int)USB_GHWCFG2_GET_NPERIO_TX_QUEUE_DEPTH(v),
    (int)USB_GHWCFG2_GET_PERIO_TX_QUEUE_DEPTH(v),
    (int)USB_GHWCFG2_GET_TOKEN_QUEUE_DEPTH(v));
}
#define USB_GHWCFG3_GET_TRANS_COUNT_WIDTH(v)     BF_EXTRACT(v, 0 , 4 )
#define USB_GHWCFG3_GET_PACKET_COUNT_WIDTH(v)    BF_EXTRACT(v, 4 , 3 )
#define USB_GHWCFG3_GET_MODE(v)                  BF_EXTRACT(v, 7 , 1 )
#define USB_GHWCFG3_GET_I2C_INTERFACE(v)         BF_EXTRACT(v, 8 , 1 )
#define USB_GHWCFG3_GET_VENDOR_CTL_INTERFACE(v)  BF_EXTRACT(v, 9 , 1 )
#define USB_GHWCFG3_GET_RM_OPT_FEATURES(v)       BF_EXTRACT(v, 10, 1 )
#define USB_GHWCFG3_GET_SYNC_RESET_TYPE(v)       BF_EXTRACT(v, 11, 1 )
#define USB_GHWCFG3_GET_UNKNOWN(v)               BF_EXTRACT(v, 12, 4 )
#define USB_GHWCFG3_GET_DFIFO_DEPTH(v)           BF_EXTRACT(v, 16, 16)
#define USB_GHWCFG3_CLR_SET_TRANS_COUNT_WIDTH(v, set)    BF_CLEAR_AND_SET(v, set, 0 , 4 )
#define USB_GHWCFG3_CLR_SET_PACKET_COUNT_WIDTH(v, set)   BF_CLEAR_AND_SET(v, set, 4 , 3 )
#define USB_GHWCFG3_CLR_SET_MODE(v, set)                 BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define USB_GHWCFG3_CLR_SET_I2C_INTERFACE(v, set)        BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define USB_GHWCFG3_CLR_SET_VENDOR_CTL_INTERFACE(v, set) BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define USB_GHWCFG3_CLR_SET_RM_OPT_FEATURES(v, set)      BF_CLEAR_AND_SET(v, set, 10, 1 )
#define USB_GHWCFG3_CLR_SET_SYNC_RESET_TYPE(v, set)      BF_CLEAR_AND_SET(v, set, 11, 1 )
#define USB_GHWCFG3_CLR_SET_UNKNOWN(v, set)              BF_CLEAR_AND_SET(v, set, 12, 4 )
#define USB_GHWCFG3_CLR_SET_DFIFO_DEPTH(v, set)          BF_CLEAR_AND_SET(v, set, 16, 16)
#define USB_GHWCFG3_CLR_TRANS_COUNT_WIDTH(v)     BF_CLEAR(v, 0 , 4 )
#define USB_GHWCFG3_CLR_PACKET_COUNT_WIDTH(v)    BF_CLEAR(v, 4 , 3 )
#define USB_GHWCFG3_CLR_MODE(v)                  BF_CLEAR(v, 7 , 1 )
#define USB_GHWCFG3_CLR_I2C_INTERFACE(v)         BF_CLEAR(v, 8 , 1 )
#define USB_GHWCFG3_CLR_VENDOR_CTL_INTERFACE(v)  BF_CLEAR(v, 9 , 1 )
#define USB_GHWCFG3_CLR_RM_OPT_FEATURES(v)       BF_CLEAR(v, 10, 1 )
#define USB_GHWCFG3_CLR_SYNC_RESET_TYPE(v)       BF_CLEAR(v, 11, 1 )
#define USB_GHWCFG3_CLR_UNKNOWN(v)               BF_CLEAR(v, 12, 4 )
#define USB_GHWCFG3_CLR_DFIFO_DEPTH(v)           BF_CLEAR(v, 16, 16)


static inline int usb_ghwcfg3_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,TRANS_COUNT_WIDTH:%x,PACKET_COUNT_WIDTH:%x,MODE:%x,I2C_INTERFACE:%x,VENDOR_CTL_INTERFACE:%x,RM_OPT_FEATURES:%x,SYNC_RESET_TYPE:%x,UNKNOWN:%x,DFIFO_DEPTH:%x",
    v,
    (int)USB_GHWCFG3_GET_TRANS_COUNT_WIDTH(v),
    (int)USB_GHWCFG3_GET_PACKET_COUNT_WIDTH(v),
    (int)USB_GHWCFG3_GET_MODE(v),
    (int)USB_GHWCFG3_GET_I2C_INTERFACE(v),
    (int)USB_GHWCFG3_GET_VENDOR_CTL_INTERFACE(v),
    (int)USB_GHWCFG3_GET_RM_OPT_FEATURES(v),
    (int)USB_GHWCFG3_GET_SYNC_RESET_TYPE(v),
    (int)USB_GHWCFG3_GET_UNKNOWN(v),
    (int)USB_GHWCFG3_GET_DFIFO_DEPTH(v));
}
#define USB_GHWCFG4_GET_NUM_PERIO_EPS(v)         BF_EXTRACT(v, 0 , 4 )
#define USB_GHWCFG4_GET_EN_PWROPT(v)             BF_EXTRACT(v, 4 , 1 )
#define USB_GHWCFG4_GET_MIN_AHB_FREQ_LESSTHAN_60(v) BF_EXTRACT(v, 5 , 1 )
#define USB_GHWCFG4_GET_PART_POWER_OFF(v)        BF_EXTRACT(v, 6 , 1 )
#define USB_GHWCFG4_GET_UNKNOWN1(v)              BF_EXTRACT(v, 7 , 7 )
#define USB_GHWCFG4_GET_HSPHY_DWIDTH(v)          BF_EXTRACT(v, 14, 2 )
#define USB_GHWCFG4_GET_NUM_CRL_EPS(v)           BF_EXTRACT(v, 16, 4 )
#define USB_GHWCFG4_GET_EN_IDDIG_FILTER(v)       BF_EXTRACT(v, 20, 1 )
#define USB_GHWCFG4_GET_EN_VBUSVALID_FILTER(v)   BF_EXTRACT(v, 21, 1 )
#define USB_GHWCFG4_GET_EN_A_VALID_FILTER(v)     BF_EXTRACT(v, 22, 1 )
#define USB_GHWCFG4_GET_EN_B_VALID_FILTER(v)     BF_EXTRACT(v, 23, 1 )
#define USB_GHWCFG4_GET_EN_SESSIONEND_FILTER(v)  BF_EXTRACT(v, 24, 1 )
#define USB_GHWCFG4_GET_EN_DED_TX_FIFO(v)        BF_EXTRACT(v, 25, 1 )
#define USB_GHWCFG4_GET_NUM_IN_EPS(v)            BF_EXTRACT(v, 26, 4 )
#define USB_GHWCFG4_GET_EN_DESC_DMA(v)           BF_EXTRACT(v, 30, 1 )
#define USB_GHWCFG4_GET_EN_DESC_DMA_DYNAMIC(v)   BF_EXTRACT(v, 31, 1 )
#define USB_GHWCFG4_CLR_SET_NUM_PERIO_EPS(v, set)        BF_CLEAR_AND_SET(v, set, 0 , 4 )
#define USB_GHWCFG4_CLR_SET_EN_PWROPT(v, set)            BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define USB_GHWCFG4_CLR_SET_MIN_AHB_FREQ_LESSTHAN_60(v, set) BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_GHWCFG4_CLR_SET_PART_POWER_OFF(v, set)       BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define USB_GHWCFG4_CLR_SET_UNKNOWN1(v, set)             BF_CLEAR_AND_SET(v, set, 7 , 7 )
#define USB_GHWCFG4_CLR_SET_HSPHY_DWIDTH(v, set)         BF_CLEAR_AND_SET(v, set, 14, 2 )
#define USB_GHWCFG4_CLR_SET_NUM_CRL_EPS(v, set)          BF_CLEAR_AND_SET(v, set, 16, 4 )
#define USB_GHWCFG4_CLR_SET_EN_IDDIG_FILTER(v, set)      BF_CLEAR_AND_SET(v, set, 20, 1 )
#define USB_GHWCFG4_CLR_SET_EN_VBUSVALID_FILTER(v, set)  BF_CLEAR_AND_SET(v, set, 21, 1 )
#define USB_GHWCFG4_CLR_SET_EN_A_VALID_FILTER(v, set)    BF_CLEAR_AND_SET(v, set, 22, 1 )
#define USB_GHWCFG4_CLR_SET_EN_B_VALID_FILTER(v, set)    BF_CLEAR_AND_SET(v, set, 23, 1 )
#define USB_GHWCFG4_CLR_SET_EN_SESSIONEND_FILTER(v, set) BF_CLEAR_AND_SET(v, set, 24, 1 )
#define USB_GHWCFG4_CLR_SET_EN_DED_TX_FIFO(v, set)       BF_CLEAR_AND_SET(v, set, 25, 1 )
#define USB_GHWCFG4_CLR_SET_NUM_IN_EPS(v, set)           BF_CLEAR_AND_SET(v, set, 26, 4 )
#define USB_GHWCFG4_CLR_SET_EN_DESC_DMA(v, set)          BF_CLEAR_AND_SET(v, set, 30, 1 )
#define USB_GHWCFG4_CLR_SET_EN_DESC_DMA_DYNAMIC(v, set)  BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_GHWCFG4_CLR_NUM_PERIO_EPS(v)         BF_CLEAR(v, 0 , 4 )
#define USB_GHWCFG4_CLR_EN_PWROPT(v)             BF_CLEAR(v, 4 , 1 )
#define USB_GHWCFG4_CLR_MIN_AHB_FREQ_LESSTHAN_60(v) BF_CLEAR(v, 5 , 1 )
#define USB_GHWCFG4_CLR_PART_POWER_OFF(v)        BF_CLEAR(v, 6 , 1 )
#define USB_GHWCFG4_CLR_UNKNOWN1(v)              BF_CLEAR(v, 7 , 7 )
#define USB_GHWCFG4_CLR_HSPHY_DWIDTH(v)          BF_CLEAR(v, 14, 2 )
#define USB_GHWCFG4_CLR_NUM_CRL_EPS(v)           BF_CLEAR(v, 16, 4 )
#define USB_GHWCFG4_CLR_EN_IDDIG_FILTER(v)       BF_CLEAR(v, 20, 1 )
#define USB_GHWCFG4_CLR_EN_VBUSVALID_FILTER(v)   BF_CLEAR(v, 21, 1 )
#define USB_GHWCFG4_CLR_EN_A_VALID_FILTER(v)     BF_CLEAR(v, 22, 1 )
#define USB_GHWCFG4_CLR_EN_B_VALID_FILTER(v)     BF_CLEAR(v, 23, 1 )
#define USB_GHWCFG4_CLR_EN_SESSIONEND_FILTER(v)  BF_CLEAR(v, 24, 1 )
#define USB_GHWCFG4_CLR_EN_DED_TX_FIFO(v)        BF_CLEAR(v, 25, 1 )
#define USB_GHWCFG4_CLR_NUM_IN_EPS(v)            BF_CLEAR(v, 26, 4 )
#define USB_GHWCFG4_CLR_EN_DESC_DMA(v)           BF_CLEAR(v, 30, 1 )
#define USB_GHWCFG4_CLR_EN_DESC_DMA_DYNAMIC(v)   BF_CLEAR(v, 31, 1 )


static inline int usb_ghwcfg4_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,NUM_PERIO_EPS:%x,EN_PWROPT:%x,MIN_AHB_FREQ_LESSTHAN_60:%x,PART_POWER_OFF:%x,UNKNOWN1:%x,HSPHY_DWIDTH:%x,NUM_CRL_EPS:%x,EN_IDDIG_FILTER:%x,EN_VBUSVALID_FILTER:%x,EN_A_VALID_FILTER:%x,EN_B_VALID_FILTER:%x,EN_SESSIONEND_FILTER:%x,EN_DED_TX_FIFO:%x,NUM_IN_EPS:%x,EN_DESC_DMA:%x,EN_DESC_DMA_DYNAMIC:%x",
    v,
    (int)USB_GHWCFG4_GET_NUM_PERIO_EPS(v),
    (int)USB_GHWCFG4_GET_EN_PWROPT(v),
    (int)USB_GHWCFG4_GET_MIN_AHB_FREQ_LESSTHAN_60(v),
    (int)USB_GHWCFG4_GET_PART_POWER_OFF(v),
    (int)USB_GHWCFG4_GET_UNKNOWN1(v),
    (int)USB_GHWCFG4_GET_HSPHY_DWIDTH(v),
    (int)USB_GHWCFG4_GET_NUM_CRL_EPS(v),
    (int)USB_GHWCFG4_GET_EN_IDDIG_FILTER(v),
    (int)USB_GHWCFG4_GET_EN_VBUSVALID_FILTER(v),
    (int)USB_GHWCFG4_GET_EN_A_VALID_FILTER(v),
    (int)USB_GHWCFG4_GET_EN_B_VALID_FILTER(v),
    (int)USB_GHWCFG4_GET_EN_SESSIONEND_FILTER(v),
    (int)USB_GHWCFG4_GET_EN_DED_TX_FIFO(v),
    (int)USB_GHWCFG4_GET_NUM_IN_EPS(v),
    (int)USB_GHWCFG4_GET_EN_DESC_DMA(v),
    (int)USB_GHWCFG4_GET_EN_DESC_DMA_DYNAMIC(v));
}
#define USB_HCFG_GET_LS_PHY_CLK_SEL(v)           BF_EXTRACT(v, 0 , 2 )
#define USB_HCFG_GET_LS_SUPP(v)                  BF_EXTRACT(v, 2 , 1 )
#define USB_HCFG_GET_EN_32KHZ_SUP(v)             BF_EXTRACT(v, 7 , 1 )
#define USB_HCFG_GET_RES_VAL_PERIOD(v)           BF_EXTRACT(v, 8 , 1 )
#define USB_HCFG_GET_DMA_DESC_ENA(v)             BF_EXTRACT(v, 23, 1 )
#define USB_HCFG_GET_FRAME_LIST_ENTR(v)          BF_EXTRACT(v, 24, 2 )
#define USB_HCFG_GET_PERIO_SCHED_ENA(v)          BF_EXTRACT(v, 26, 1 )
#define USB_HCFG_GET_PERIO_SCHED_STA(v)          BF_EXTRACT(v, 27, 1 )
#define USB_HCFG_GET_MODE_CHNG_TIME(v)           BF_EXTRACT(v, 31, 1 )
#define USB_HCFG_CLR_SET_LS_PHY_CLK_SEL(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 2 )
#define USB_HCFG_CLR_SET_LS_SUPP(v, set)                 BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define USB_HCFG_CLR_SET_EN_32KHZ_SUP(v, set)            BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define USB_HCFG_CLR_SET_RES_VAL_PERIOD(v, set)          BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define USB_HCFG_CLR_SET_DMA_DESC_ENA(v, set)            BF_CLEAR_AND_SET(v, set, 23, 1 )
#define USB_HCFG_CLR_SET_FRAME_LIST_ENTR(v, set)         BF_CLEAR_AND_SET(v, set, 24, 2 )
#define USB_HCFG_CLR_SET_PERIO_SCHED_ENA(v, set)         BF_CLEAR_AND_SET(v, set, 26, 1 )
#define USB_HCFG_CLR_SET_PERIO_SCHED_STA(v, set)         BF_CLEAR_AND_SET(v, set, 27, 1 )
#define USB_HCFG_CLR_SET_MODE_CHNG_TIME(v, set)          BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_HCFG_CLR_LS_PHY_CLK_SEL(v)           BF_CLEAR(v, 0 , 2 )
#define USB_HCFG_CLR_LS_SUPP(v)                  BF_CLEAR(v, 2 , 1 )
#define USB_HCFG_CLR_EN_32KHZ_SUP(v)             BF_CLEAR(v, 7 , 1 )
#define USB_HCFG_CLR_RES_VAL_PERIOD(v)           BF_CLEAR(v, 8 , 1 )
#define USB_HCFG_CLR_DMA_DESC_ENA(v)             BF_CLEAR(v, 23, 1 )
#define USB_HCFG_CLR_FRAME_LIST_ENTR(v)          BF_CLEAR(v, 24, 2 )
#define USB_HCFG_CLR_PERIO_SCHED_ENA(v)          BF_CLEAR(v, 26, 1 )
#define USB_HCFG_CLR_PERIO_SCHED_STA(v)          BF_CLEAR(v, 27, 1 )
#define USB_HCFG_CLR_MODE_CHNG_TIME(v)           BF_CLEAR(v, 31, 1 )


static inline int usb_hcfg_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,LS_PHY_CLK_SEL:%x,LS_SUPP:%x,EN_32KHZ_SUP:%x,RES_VAL_PERIOD:%x,DMA_DESC_ENA:%x,FRAME_LIST_ENTR:%x,PERIO_SCHED_ENA:%x,PERIO_SCHED_STA:%x,MODE_CHNG_TIME:%x",
    v,
    (int)USB_HCFG_GET_LS_PHY_CLK_SEL(v),
    (int)USB_HCFG_GET_LS_SUPP(v),
    (int)USB_HCFG_GET_EN_32KHZ_SUP(v),
    (int)USB_HCFG_GET_RES_VAL_PERIOD(v),
    (int)USB_HCFG_GET_DMA_DESC_ENA(v),
    (int)USB_HCFG_GET_FRAME_LIST_ENTR(v),
    (int)USB_HCFG_GET_PERIO_SCHED_ENA(v),
    (int)USB_HCFG_GET_PERIO_SCHED_STA(v),
    (int)USB_HCFG_GET_MODE_CHNG_TIME(v));
}
#define USB_GRSTCTL_GET_C_SFT_RST(v)             BF_EXTRACT(v, 0 , 1 )
#define USB_GRSTCTL_GET_H_SFT_RST(v)             BF_EXTRACT(v, 1 , 1 )
#define USB_GRSTCTL_GET_FRM_CNTR_RST(v)          BF_EXTRACT(v, 2 , 1 )
#define USB_GRSTCTL_GET_INT_TKN_Q_FLSH(v)        BF_EXTRACT(v, 3 , 1 )
#define USB_GRSTCTL_GET_RXF_FLSH(v)              BF_EXTRACT(v, 4 , 1 )
#define USB_GRSTCTL_GET_TXF_FLSH(v)              BF_EXTRACT(v, 5 , 1 )
#define USB_GRSTCTL_GET_TXF_NUM(v)               BF_EXTRACT(v, 6 , 5 )
#define USB_GRSTCTL_GET_UNKNOWN(v)               BF_EXTRACT(v, 11, 1 )
#define USB_GRSTCTL_GET_DMA_REQ(v)               BF_EXTRACT(v, 30, 1 )
#define USB_GRSTCTL_GET_AHB_IDLE(v)              BF_EXTRACT(v, 31, 1 )
#define USB_GRSTCTL_CLR_SET_C_SFT_RST(v, set)            BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define USB_GRSTCTL_CLR_SET_H_SFT_RST(v, set)            BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define USB_GRSTCTL_CLR_SET_FRM_CNTR_RST(v, set)         BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define USB_GRSTCTL_CLR_SET_INT_TKN_Q_FLSH(v, set)       BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define USB_GRSTCTL_CLR_SET_RXF_FLSH(v, set)             BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define USB_GRSTCTL_CLR_SET_TXF_FLSH(v, set)             BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_GRSTCTL_CLR_SET_TXF_NUM(v, set)              BF_CLEAR_AND_SET(v, set, 6 , 5 )
#define USB_GRSTCTL_CLR_SET_UNKNOWN(v, set)              BF_CLEAR_AND_SET(v, set, 11, 1 )
#define USB_GRSTCTL_CLR_SET_DMA_REQ(v, set)              BF_CLEAR_AND_SET(v, set, 30, 1 )
#define USB_GRSTCTL_CLR_SET_AHB_IDLE(v, set)             BF_CLEAR_AND_SET(v, set, 31, 1 )
#define USB_GRSTCTL_CLR_C_SFT_RST(v)             BF_CLEAR(v, 0 , 1 )
#define USB_GRSTCTL_CLR_H_SFT_RST(v)             BF_CLEAR(v, 1 , 1 )
#define USB_GRSTCTL_CLR_FRM_CNTR_RST(v)          BF_CLEAR(v, 2 , 1 )
#define USB_GRSTCTL_CLR_INT_TKN_Q_FLSH(v)        BF_CLEAR(v, 3 , 1 )
#define USB_GRSTCTL_CLR_RXF_FLSH(v)              BF_CLEAR(v, 4 , 1 )
#define USB_GRSTCTL_CLR_TXF_FLSH(v)              BF_CLEAR(v, 5 , 1 )
#define USB_GRSTCTL_CLR_TXF_NUM(v)               BF_CLEAR(v, 6 , 5 )
#define USB_GRSTCTL_CLR_UNKNOWN(v)               BF_CLEAR(v, 11, 1 )
#define USB_GRSTCTL_CLR_DMA_REQ(v)               BF_CLEAR(v, 30, 1 )
#define USB_GRSTCTL_CLR_AHB_IDLE(v)              BF_CLEAR(v, 31, 1 )


static inline int usb_grstctl_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,C_SFT_RST:%x,H_SFT_RST:%x,FRM_CNTR_RST:%x,INT_TKN_Q_FLSH:%x,RXF_FLSH:%x,TXF_FLSH:%x,TXF_NUM:%x,UNKNOWN:%x,DMA_REQ:%x,AHB_IDLE:%x",
    v,
    (int)USB_GRSTCTL_GET_C_SFT_RST(v),
    (int)USB_GRSTCTL_GET_H_SFT_RST(v),
    (int)USB_GRSTCTL_GET_FRM_CNTR_RST(v),
    (int)USB_GRSTCTL_GET_INT_TKN_Q_FLSH(v),
    (int)USB_GRSTCTL_GET_RXF_FLSH(v),
    (int)USB_GRSTCTL_GET_TXF_FLSH(v),
    (int)USB_GRSTCTL_GET_TXF_NUM(v),
    (int)USB_GRSTCTL_GET_UNKNOWN(v),
    (int)USB_GRSTCTL_GET_DMA_REQ(v),
    (int)USB_GRSTCTL_GET_AHB_IDLE(v));
}
#define USB_HPRT_GET_CONN_STS(v)                 BF_EXTRACT(v, 0 , 1 )
#define USB_HPRT_GET_CONN_DET(v)                 BF_EXTRACT(v, 1 , 1 )
#define USB_HPRT_GET_ENA(v)                      BF_EXTRACT(v, 2 , 1 )
#define USB_HPRT_GET_EN_CHNG(v)                  BF_EXTRACT(v, 3 , 1 )
#define USB_HPRT_GET_OVR_CURR_ACT(v)             BF_EXTRACT(v, 4 , 1 )
#define USB_HPRT_GET_OVR_CURR_CHNG(v)            BF_EXTRACT(v, 5 , 1 )
#define USB_HPRT_GET_RES(v)                      BF_EXTRACT(v, 6 , 1 )
#define USB_HPRT_GET_SUSP(v)                     BF_EXTRACT(v, 7 , 1 )
#define USB_HPRT_GET_RST(v)                      BF_EXTRACT(v, 8 , 1 )
#define USB_HPRT_GET_RES0(v)                     BF_EXTRACT(v, 9 , 1 )
#define USB_HPRT_GET_LN_STS(v)                   BF_EXTRACT(v, 10, 2 )
#define USB_HPRT_GET_PWR(v)                      BF_EXTRACT(v, 12, 1 )
#define USB_HPRT_GET_TST_CTL(v)                  BF_EXTRACT(v, 13, 4 )
#define USB_HPRT_GET_SPD(v)                      BF_EXTRACT(v, 17, 2 )
#define USB_HPRT_CLR_SET_CONN_STS(v, set)                BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define USB_HPRT_CLR_SET_CONN_DET(v, set)                BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define USB_HPRT_CLR_SET_ENA(v, set)                     BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define USB_HPRT_CLR_SET_EN_CHNG(v, set)                 BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define USB_HPRT_CLR_SET_OVR_CURR_ACT(v, set)            BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define USB_HPRT_CLR_SET_OVR_CURR_CHNG(v, set)           BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_HPRT_CLR_SET_RES(v, set)                     BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define USB_HPRT_CLR_SET_SUSP(v, set)                    BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define USB_HPRT_CLR_SET_RST(v, set)                     BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define USB_HPRT_CLR_SET_RES0(v, set)                    BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define USB_HPRT_CLR_SET_LN_STS(v, set)                  BF_CLEAR_AND_SET(v, set, 10, 2 )
#define USB_HPRT_CLR_SET_PWR(v, set)                     BF_CLEAR_AND_SET(v, set, 12, 1 )
#define USB_HPRT_CLR_SET_TST_CTL(v, set)                 BF_CLEAR_AND_SET(v, set, 13, 4 )
#define USB_HPRT_CLR_SET_SPD(v, set)                     BF_CLEAR_AND_SET(v, set, 17, 2 )
#define USB_HPRT_CLR_CONN_STS(v)                 BF_CLEAR(v, 0 , 1 )
#define USB_HPRT_CLR_CONN_DET(v)                 BF_CLEAR(v, 1 , 1 )
#define USB_HPRT_CLR_ENA(v)                      BF_CLEAR(v, 2 , 1 )
#define USB_HPRT_CLR_EN_CHNG(v)                  BF_CLEAR(v, 3 , 1 )
#define USB_HPRT_CLR_OVR_CURR_ACT(v)             BF_CLEAR(v, 4 , 1 )
#define USB_HPRT_CLR_OVR_CURR_CHNG(v)            BF_CLEAR(v, 5 , 1 )
#define USB_HPRT_CLR_RES(v)                      BF_CLEAR(v, 6 , 1 )
#define USB_HPRT_CLR_SUSP(v)                     BF_CLEAR(v, 7 , 1 )
#define USB_HPRT_CLR_RST(v)                      BF_CLEAR(v, 8 , 1 )
#define USB_HPRT_CLR_RES0(v)                     BF_CLEAR(v, 9 , 1 )
#define USB_HPRT_CLR_LN_STS(v)                   BF_CLEAR(v, 10, 2 )
#define USB_HPRT_CLR_PWR(v)                      BF_CLEAR(v, 12, 1 )
#define USB_HPRT_CLR_TST_CTL(v)                  BF_CLEAR(v, 13, 4 )
#define USB_HPRT_CLR_SPD(v)                      BF_CLEAR(v, 17, 2 )


static inline int usb_hprt_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CONN_STS:%x,CONN_DET:%x,ENA:%x,EN_CHNG:%x,OVR_CURR_ACT:%x,OVR_CURR_CHNG:%x,RES:%x,SUSP:%x,RST:%x,RES0:%x,LN_STS:%x,PWR:%x,TST_CTL:%x,SPD:%x",
    v,
    (int)USB_HPRT_GET_CONN_STS(v),
    (int)USB_HPRT_GET_CONN_DET(v),
    (int)USB_HPRT_GET_ENA(v),
    (int)USB_HPRT_GET_EN_CHNG(v),
    (int)USB_HPRT_GET_OVR_CURR_ACT(v),
    (int)USB_HPRT_GET_OVR_CURR_CHNG(v),
    (int)USB_HPRT_GET_RES(v),
    (int)USB_HPRT_GET_SUSP(v),
    (int)USB_HPRT_GET_RST(v),
    (int)USB_HPRT_GET_RES0(v),
    (int)USB_HPRT_GET_LN_STS(v),
    (int)USB_HPRT_GET_PWR(v),
    (int)USB_HPRT_GET_TST_CTL(v),
    (int)USB_HPRT_GET_SPD(v));
}
#define USB_PCGCR_GET_STOP_PCLK(v)               BF_EXTRACT(v, 0 , 1 )
#define USB_PCGCR_GET_GATE_HCLK(v)               BF_EXTRACT(v, 1 , 1 )
#define USB_PCGCR_GET_PWR_CLMP(v)                BF_EXTRACT(v, 2 , 1 )
#define USB_PCGCR_GET_RST_PDWN_MODULE(v)         BF_EXTRACT(v, 3 , 1 )
#define USB_PCGCR_GET_PHY_SUSPENDED(v)           BF_EXTRACT(v, 4 , 1 )
#define USB_PCGCR_GET_EN_SLP_CLK_GATE(v)         BF_EXTRACT(v, 5 , 1 )
#define USB_PCGCR_GET_PHY_SLEEPING(v)            BF_EXTRACT(v, 6 , 1 )
#define USB_PCGCR_GET_DEEP_SLEEP(v)              BF_EXTRACT(v, 7 , 1 )
#define USB_PCGCR_CLR_SET_STOP_PCLK(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define USB_PCGCR_CLR_SET_GATE_HCLK(v, set)              BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define USB_PCGCR_CLR_SET_PWR_CLMP(v, set)               BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define USB_PCGCR_CLR_SET_RST_PDWN_MODULE(v, set)        BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define USB_PCGCR_CLR_SET_PHY_SUSPENDED(v, set)          BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define USB_PCGCR_CLR_SET_EN_SLP_CLK_GATE(v, set)        BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define USB_PCGCR_CLR_SET_PHY_SLEEPING(v, set)           BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define USB_PCGCR_CLR_SET_DEEP_SLEEP(v, set)             BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define USB_PCGCR_CLR_STOP_PCLK(v)               BF_CLEAR(v, 0 , 1 )
#define USB_PCGCR_CLR_GATE_HCLK(v)               BF_CLEAR(v, 1 , 1 )
#define USB_PCGCR_CLR_PWR_CLMP(v)                BF_CLEAR(v, 2 , 1 )
#define USB_PCGCR_CLR_RST_PDWN_MODULE(v)         BF_CLEAR(v, 3 , 1 )
#define USB_PCGCR_CLR_PHY_SUSPENDED(v)           BF_CLEAR(v, 4 , 1 )
#define USB_PCGCR_CLR_EN_SLP_CLK_GATE(v)         BF_CLEAR(v, 5 , 1 )
#define USB_PCGCR_CLR_PHY_SLEEPING(v)            BF_CLEAR(v, 6 , 1 )
#define USB_PCGCR_CLR_DEEP_SLEEP(v)              BF_CLEAR(v, 7 , 1 )


static inline int usb_pcgcr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,STOP_PCLK:%x,GATE_HCLK:%x,PWR_CLMP:%x,RST_PDWN_MODULE:%x,PHY_SUSPENDED:%x,EN_SLP_CLK_GATE:%x,PHY_SLEEPING:%x,DEEP_SLEEP:%x",
    v,
    (int)USB_PCGCR_GET_STOP_PCLK(v),
    (int)USB_PCGCR_GET_GATE_HCLK(v),
    (int)USB_PCGCR_GET_PWR_CLMP(v),
    (int)USB_PCGCR_GET_RST_PDWN_MODULE(v),
    (int)USB_PCGCR_GET_PHY_SUSPENDED(v),
    (int)USB_PCGCR_GET_EN_SLP_CLK_GATE(v),
    (int)USB_PCGCR_GET_PHY_SLEEPING(v),
    (int)USB_PCGCR_GET_DEEP_SLEEP(v));
}
