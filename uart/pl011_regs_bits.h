#pragma once

#define PL011_UARTDR_GET_DATA(v)                 BF_EXTRACT(v, 0 , 8 )
#define PL011_UARTDR_GET_FE(v)                   BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTDR_GET_PE(v)                   BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTDR_GET_BE(v)                   BF_EXTRACT(v, 10, 1 )
#define PL011_UARTDR_GET_OE(v)                   BF_EXTRACT(v, 11, 1 )
#define PL011_UARTDR_CLR_SET_DATA(v, set)                BF_CLEAR_AND_SET(v, set, 0 , 8 )
#define PL011_UARTDR_CLR_SET_FE(v, set)                  BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTDR_CLR_SET_PE(v, set)                  BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTDR_CLR_SET_BE(v, set)                  BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTDR_CLR_SET_OE(v, set)                  BF_CLEAR_AND_SET(v, set, 11, 1 )
#define PL011_UARTDR_CLR_DATA(v)                 BF_CLEAR(v, 0 , 8 )
#define PL011_UARTDR_CLR_FE(v)                   BF_CLEAR(v, 8 , 1 )
#define PL011_UARTDR_CLR_PE(v)                   BF_CLEAR(v, 9 , 1 )
#define PL011_UARTDR_CLR_BE(v)                   BF_CLEAR(v, 10, 1 )
#define PL011_UARTDR_CLR_OE(v)                   BF_CLEAR(v, 11, 1 )


static inline int pl011_uartdr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,DATA:%x,FE:%x,PE:%x,BE:%x,OE:%x",
    v,
    (int)PL011_UARTDR_GET_DATA(v),
    (int)PL011_UARTDR_GET_FE(v),
    (int)PL011_UARTDR_GET_PE(v),
    (int)PL011_UARTDR_GET_BE(v),
    (int)PL011_UARTDR_GET_OE(v));
}
#define PL011_UARTFR_GET_CTS(v)                  BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTFR_GET_DSR(v)                  BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTFR_GET_DCD(v)                  BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTFR_GET_BUSY(v)                 BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTFR_GET_RXFE(v)                 BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTFR_GET_TXFF(v)                 BF_EXTRACT(v, 5 , 1 )
#define PL011_UARTFR_GET_RXFF(v)                 BF_EXTRACT(v, 6 , 1 )
#define PL011_UARTFR_GET_TXFE(v)                 BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTFR_GET_RI(v)                   BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTFR_CLR_SET_CTS(v, set)                 BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTFR_CLR_SET_DSR(v, set)                 BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTFR_CLR_SET_DCD(v, set)                 BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTFR_CLR_SET_BUSY(v, set)                BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTFR_CLR_SET_RXFE(v, set)                BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTFR_CLR_SET_TXFF(v, set)                BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define PL011_UARTFR_CLR_SET_RXFF(v, set)                BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define PL011_UARTFR_CLR_SET_TXFE(v, set)                BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTFR_CLR_SET_RI(v, set)                  BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTFR_CLR_CTS(v)                  BF_CLEAR(v, 0 , 1 )
#define PL011_UARTFR_CLR_DSR(v)                  BF_CLEAR(v, 1 , 1 )
#define PL011_UARTFR_CLR_DCD(v)                  BF_CLEAR(v, 2 , 1 )
#define PL011_UARTFR_CLR_BUSY(v)                 BF_CLEAR(v, 3 , 1 )
#define PL011_UARTFR_CLR_RXFE(v)                 BF_CLEAR(v, 4 , 1 )
#define PL011_UARTFR_CLR_TXFF(v)                 BF_CLEAR(v, 5 , 1 )
#define PL011_UARTFR_CLR_RXFF(v)                 BF_CLEAR(v, 6 , 1 )
#define PL011_UARTFR_CLR_TXFE(v)                 BF_CLEAR(v, 7 , 1 )
#define PL011_UARTFR_CLR_RI(v)                   BF_CLEAR(v, 8 , 1 )

static inline int pl011_uartfr_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTFR_GET_CTS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_DSR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDSR", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_DCD(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCD", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_BUSY(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBUSY", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_RXFE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXFE", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_TXFF(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXFF", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_RXFF(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXFF", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_TXFE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXFE", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTFR_GET_RI(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRI", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uartfr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,CTS:%x,DSR:%x,DCD:%x,BUSY:%x,RXFE:%x,TXFF:%x,RXFF:%x,TXFE:%x,RI:%x",
    v,
    (int)PL011_UARTFR_GET_CTS(v),
    (int)PL011_UARTFR_GET_DSR(v),
    (int)PL011_UARTFR_GET_DCD(v),
    (int)PL011_UARTFR_GET_BUSY(v),
    (int)PL011_UARTFR_GET_RXFE(v),
    (int)PL011_UARTFR_GET_TXFF(v),
    (int)PL011_UARTFR_GET_RXFF(v),
    (int)PL011_UARTFR_GET_TXFE(v),
    (int)PL011_UARTFR_GET_RI(v));
}
#define PL011_UARTLCR_H_GET_BRK(v)               BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTLCR_H_GET_PEN(v)               BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTLCR_H_GET_EPS(v)               BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTLCR_H_GET_STP2(v)              BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTLCR_H_GET_FEN(v)               BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTLCR_H_GET_WLEN(v)              BF_EXTRACT(v, 5 , 2 )
#define PL011_UARTLCR_H_GET_SPS(v)               BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTLCR_H_CLR_SET_BRK(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTLCR_H_CLR_SET_PEN(v, set)              BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTLCR_H_CLR_SET_EPS(v, set)              BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTLCR_H_CLR_SET_STP2(v, set)             BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTLCR_H_CLR_SET_FEN(v, set)              BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTLCR_H_CLR_SET_WLEN(v, set)             BF_CLEAR_AND_SET(v, set, 5 , 2 )
#define PL011_UARTLCR_H_CLR_SET_SPS(v, set)              BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTLCR_H_CLR_BRK(v)               BF_CLEAR(v, 0 , 1 )
#define PL011_UARTLCR_H_CLR_PEN(v)               BF_CLEAR(v, 1 , 1 )
#define PL011_UARTLCR_H_CLR_EPS(v)               BF_CLEAR(v, 2 , 1 )
#define PL011_UARTLCR_H_CLR_STP2(v)              BF_CLEAR(v, 3 , 1 )
#define PL011_UARTLCR_H_CLR_FEN(v)               BF_CLEAR(v, 4 , 1 )
#define PL011_UARTLCR_H_CLR_WLEN(v)              BF_CLEAR(v, 5 , 2 )
#define PL011_UARTLCR_H_CLR_SPS(v)               BF_CLEAR(v, 7 , 1 )


static inline int pl011_uartlcr_h_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,BRK:%x,PEN:%x,EPS:%x,STP2:%x,FEN:%x,WLEN:%x,SPS:%x",
    v,
    (int)PL011_UARTLCR_H_GET_BRK(v),
    (int)PL011_UARTLCR_H_GET_PEN(v),
    (int)PL011_UARTLCR_H_GET_EPS(v),
    (int)PL011_UARTLCR_H_GET_STP2(v),
    (int)PL011_UARTLCR_H_GET_FEN(v),
    (int)PL011_UARTLCR_H_GET_WLEN(v),
    (int)PL011_UARTLCR_H_GET_SPS(v));
}
#define PL011_UARTCR_GET_UARTEN(v)               BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTCR_GET_SIREN(v)                BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTCR_GET_SIRLP(v)                BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTCR_GET_LBE(v)                  BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTCR_GET_TXE(v)                  BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTCR_GET_RXE(v)                  BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTCR_GET_DTR(v)                  BF_EXTRACT(v, 10, 1 )
#define PL011_UARTCR_GET_RTS(v)                  BF_EXTRACT(v, 11, 1 )
#define PL011_UARTCR_GET_OUT1(v)                 BF_EXTRACT(v, 12, 1 )
#define PL011_UARTCR_GET_OUT2(v)                 BF_EXTRACT(v, 13, 1 )
#define PL011_UARTCR_GET_RTSEN(v)                BF_EXTRACT(v, 14, 1 )
#define PL011_UARTCR_GET_CTSEN(v)                BF_EXTRACT(v, 15, 1 )
#define PL011_UARTCR_CLR_SET_UARTEN(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTCR_CLR_SET_SIREN(v, set)               BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTCR_CLR_SET_SIRLP(v, set)               BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTCR_CLR_SET_LBE(v, set)                 BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTCR_CLR_SET_TXE(v, set)                 BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTCR_CLR_SET_RXE(v, set)                 BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTCR_CLR_SET_DTR(v, set)                 BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTCR_CLR_SET_RTS(v, set)                 BF_CLEAR_AND_SET(v, set, 11, 1 )
#define PL011_UARTCR_CLR_SET_OUT1(v, set)                BF_CLEAR_AND_SET(v, set, 12, 1 )
#define PL011_UARTCR_CLR_SET_OUT2(v, set)                BF_CLEAR_AND_SET(v, set, 13, 1 )
#define PL011_UARTCR_CLR_SET_RTSEN(v, set)               BF_CLEAR_AND_SET(v, set, 14, 1 )
#define PL011_UARTCR_CLR_SET_CTSEN(v, set)               BF_CLEAR_AND_SET(v, set, 15, 1 )
#define PL011_UARTCR_CLR_UARTEN(v)               BF_CLEAR(v, 0 , 1 )
#define PL011_UARTCR_CLR_SIREN(v)                BF_CLEAR(v, 1 , 1 )
#define PL011_UARTCR_CLR_SIRLP(v)                BF_CLEAR(v, 2 , 1 )
#define PL011_UARTCR_CLR_LBE(v)                  BF_CLEAR(v, 7 , 1 )
#define PL011_UARTCR_CLR_TXE(v)                  BF_CLEAR(v, 8 , 1 )
#define PL011_UARTCR_CLR_RXE(v)                  BF_CLEAR(v, 9 , 1 )
#define PL011_UARTCR_CLR_DTR(v)                  BF_CLEAR(v, 10, 1 )
#define PL011_UARTCR_CLR_RTS(v)                  BF_CLEAR(v, 11, 1 )
#define PL011_UARTCR_CLR_OUT1(v)                 BF_CLEAR(v, 12, 1 )
#define PL011_UARTCR_CLR_OUT2(v)                 BF_CLEAR(v, 13, 1 )
#define PL011_UARTCR_CLR_RTSEN(v)                BF_CLEAR(v, 14, 1 )
#define PL011_UARTCR_CLR_CTSEN(v)                BF_CLEAR(v, 15, 1 )

static inline int pl011_uartcr_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTCR_GET_UARTEN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sUARTEN", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_SIREN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sSIREN", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_SIRLP(v)) {
    n += snprintf(buf + n, bufsz - n, "%sSIRLP", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_LBE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sLBE", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_TXE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXE", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_RXE(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXE", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_DTR(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDTR", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_RTS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_OUT1(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOUT1", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_OUT2(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOUT2", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_RTSEN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTSEN", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTCR_GET_CTSEN(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTSEN", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uartcr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,UARTEN:%x,SIREN:%x,SIRLP:%x,LBE:%x,TXE:%x,RXE:%x,DTR:%x,RTS:%x,OUT1:%x,OUT2:%x,RTSEN:%x,CTSEN:%x",
    v,
    (int)PL011_UARTCR_GET_UARTEN(v),
    (int)PL011_UARTCR_GET_SIREN(v),
    (int)PL011_UARTCR_GET_SIRLP(v),
    (int)PL011_UARTCR_GET_LBE(v),
    (int)PL011_UARTCR_GET_TXE(v),
    (int)PL011_UARTCR_GET_RXE(v),
    (int)PL011_UARTCR_GET_DTR(v),
    (int)PL011_UARTCR_GET_RTS(v),
    (int)PL011_UARTCR_GET_OUT1(v),
    (int)PL011_UARTCR_GET_OUT2(v),
    (int)PL011_UARTCR_GET_RTSEN(v),
    (int)PL011_UARTCR_GET_CTSEN(v));
}
#define PL011_UARTIFLS_GET_TXIFLSEL(v)           BF_EXTRACT(v, 0 , 3 )
#define PL011_UARTIFLS_GET_RXIFLSEL(v)           BF_EXTRACT(v, 3 , 3 )
#define PL011_UARTIFLS_CLR_SET_TXIFLSEL(v, set)          BF_CLEAR_AND_SET(v, set, 0 , 3 )
#define PL011_UARTIFLS_CLR_SET_RXIFLSEL(v, set)          BF_CLEAR_AND_SET(v, set, 3 , 3 )
#define PL011_UARTIFLS_CLR_TXIFLSEL(v)           BF_CLEAR(v, 0 , 3 )
#define PL011_UARTIFLS_CLR_RXIFLSEL(v)           BF_CLEAR(v, 3 , 3 )


static inline int pl011_uartifls_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,TXIFLSEL:%x,RXIFLSEL:%x",
    v,
    (int)PL011_UARTIFLS_GET_TXIFLSEL(v),
    (int)PL011_UARTIFLS_GET_RXIFLSEL(v));
}
#define PL011_UARTIMSC_GET_RIMIM(v)              BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTIMSC_GET_CTSMIM(v)             BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTIMSC_GET_DCDMIM(v)             BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTIMSC_GET_DSRMIM(v)             BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTIMSC_GET_RXIM(v)               BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTIMSC_GET_TXIM(v)               BF_EXTRACT(v, 5 , 1 )
#define PL011_UARTIMSC_GET_RTIM(v)               BF_EXTRACT(v, 6 , 1 )
#define PL011_UARTIMSC_GET_FEIM(v)               BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTIMSC_GET_PEIM(v)               BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTIMSC_GET_BEIM(v)               BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTIMSC_GET_OEIM(v)               BF_EXTRACT(v, 10, 1 )
#define PL011_UARTIMSC_CLR_SET_RIMIM(v, set)             BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTIMSC_CLR_SET_CTSMIM(v, set)            BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTIMSC_CLR_SET_DCDMIM(v, set)            BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTIMSC_CLR_SET_DSRMIM(v, set)            BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTIMSC_CLR_SET_RXIM(v, set)              BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTIMSC_CLR_SET_TXIM(v, set)              BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define PL011_UARTIMSC_CLR_SET_RTIM(v, set)              BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define PL011_UARTIMSC_CLR_SET_FEIM(v, set)              BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTIMSC_CLR_SET_PEIM(v, set)              BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTIMSC_CLR_SET_BEIM(v, set)              BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTIMSC_CLR_SET_OEIM(v, set)              BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTIMSC_CLR_RIMIM(v)              BF_CLEAR(v, 0 , 1 )
#define PL011_UARTIMSC_CLR_CTSMIM(v)             BF_CLEAR(v, 1 , 1 )
#define PL011_UARTIMSC_CLR_DCDMIM(v)             BF_CLEAR(v, 2 , 1 )
#define PL011_UARTIMSC_CLR_DSRMIM(v)             BF_CLEAR(v, 3 , 1 )
#define PL011_UARTIMSC_CLR_RXIM(v)               BF_CLEAR(v, 4 , 1 )
#define PL011_UARTIMSC_CLR_TXIM(v)               BF_CLEAR(v, 5 , 1 )
#define PL011_UARTIMSC_CLR_RTIM(v)               BF_CLEAR(v, 6 , 1 )
#define PL011_UARTIMSC_CLR_FEIM(v)               BF_CLEAR(v, 7 , 1 )
#define PL011_UARTIMSC_CLR_PEIM(v)               BF_CLEAR(v, 8 , 1 )
#define PL011_UARTIMSC_CLR_BEIM(v)               BF_CLEAR(v, 9 , 1 )
#define PL011_UARTIMSC_CLR_OEIM(v)               BF_CLEAR(v, 10, 1 )

static inline int pl011_uartimsc_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTIMSC_GET_RIMIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRIMIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_CTSMIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTSMIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_DCDMIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCDMIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_DSRMIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDSRMIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_RXIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_TXIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_RTIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_FEIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sFEIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_PEIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sPEIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_BEIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBEIM", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTIMSC_GET_OEIM(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOEIM", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uartimsc_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RIMIM:%x,CTSMIM:%x,DCDMIM:%x,DSRMIM:%x,RXIM:%x,TXIM:%x,RTIM:%x,FEIM:%x,PEIM:%x,BEIM:%x,OEIM:%x",
    v,
    (int)PL011_UARTIMSC_GET_RIMIM(v),
    (int)PL011_UARTIMSC_GET_CTSMIM(v),
    (int)PL011_UARTIMSC_GET_DCDMIM(v),
    (int)PL011_UARTIMSC_GET_DSRMIM(v),
    (int)PL011_UARTIMSC_GET_RXIM(v),
    (int)PL011_UARTIMSC_GET_TXIM(v),
    (int)PL011_UARTIMSC_GET_RTIM(v),
    (int)PL011_UARTIMSC_GET_FEIM(v),
    (int)PL011_UARTIMSC_GET_PEIM(v),
    (int)PL011_UARTIMSC_GET_BEIM(v),
    (int)PL011_UARTIMSC_GET_OEIM(v));
}
#define PL011_UARTRIS_GET_RIRMIS(v)              BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTRIS_GET_CTSRMIS(v)             BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTRIS_GET_DCDRMIS(v)             BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTRIS_GET_DSRRMIS(v)             BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTRIS_GET_RXRIS(v)               BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTRIS_GET_TXRIS(v)               BF_EXTRACT(v, 5 , 1 )
#define PL011_UARTRIS_GET_RTRIS(v)               BF_EXTRACT(v, 6 , 1 )
#define PL011_UARTRIS_GET_FERIS(v)               BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTRIS_GET_PERIS(v)               BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTRIS_GET_BERIS(v)               BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTRIS_GET_OERIS(v)               BF_EXTRACT(v, 10, 1 )
#define PL011_UARTRIS_CLR_SET_RIRMIS(v, set)             BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTRIS_CLR_SET_CTSRMIS(v, set)            BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTRIS_CLR_SET_DCDRMIS(v, set)            BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTRIS_CLR_SET_DSRRMIS(v, set)            BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTRIS_CLR_SET_RXRIS(v, set)              BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTRIS_CLR_SET_TXRIS(v, set)              BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define PL011_UARTRIS_CLR_SET_RTRIS(v, set)              BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define PL011_UARTRIS_CLR_SET_FERIS(v, set)              BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTRIS_CLR_SET_PERIS(v, set)              BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTRIS_CLR_SET_BERIS(v, set)              BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTRIS_CLR_SET_OERIS(v, set)              BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTRIS_CLR_RIRMIS(v)              BF_CLEAR(v, 0 , 1 )
#define PL011_UARTRIS_CLR_CTSRMIS(v)             BF_CLEAR(v, 1 , 1 )
#define PL011_UARTRIS_CLR_DCDRMIS(v)             BF_CLEAR(v, 2 , 1 )
#define PL011_UARTRIS_CLR_DSRRMIS(v)             BF_CLEAR(v, 3 , 1 )
#define PL011_UARTRIS_CLR_RXRIS(v)               BF_CLEAR(v, 4 , 1 )
#define PL011_UARTRIS_CLR_TXRIS(v)               BF_CLEAR(v, 5 , 1 )
#define PL011_UARTRIS_CLR_RTRIS(v)               BF_CLEAR(v, 6 , 1 )
#define PL011_UARTRIS_CLR_FERIS(v)               BF_CLEAR(v, 7 , 1 )
#define PL011_UARTRIS_CLR_PERIS(v)               BF_CLEAR(v, 8 , 1 )
#define PL011_UARTRIS_CLR_BERIS(v)               BF_CLEAR(v, 9 , 1 )
#define PL011_UARTRIS_CLR_OERIS(v)               BF_CLEAR(v, 10, 1 )

static inline int pl011_uartris_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTRIS_GET_RIRMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRIRMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_CTSRMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTSRMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_DCDRMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCDRMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_DSRRMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDSRRMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_RXRIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXRIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_TXRIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXRIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_RTRIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTRIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_FERIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sFERIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_PERIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sPERIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_BERIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBERIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTRIS_GET_OERIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOERIS", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uartris_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RIRMIS:%x,CTSRMIS:%x,DCDRMIS:%x,DSRRMIS:%x,RXRIS:%x,TXRIS:%x,RTRIS:%x,FERIS:%x,PERIS:%x,BERIS:%x,OERIS:%x",
    v,
    (int)PL011_UARTRIS_GET_RIRMIS(v),
    (int)PL011_UARTRIS_GET_CTSRMIS(v),
    (int)PL011_UARTRIS_GET_DCDRMIS(v),
    (int)PL011_UARTRIS_GET_DSRRMIS(v),
    (int)PL011_UARTRIS_GET_RXRIS(v),
    (int)PL011_UARTRIS_GET_TXRIS(v),
    (int)PL011_UARTRIS_GET_RTRIS(v),
    (int)PL011_UARTRIS_GET_FERIS(v),
    (int)PL011_UARTRIS_GET_PERIS(v),
    (int)PL011_UARTRIS_GET_BERIS(v),
    (int)PL011_UARTRIS_GET_OERIS(v));
}
#define PL011_UARTMIS_GET_RIMMIS(v)              BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTMIS_GET_CTSMMIS(v)             BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTMIS_GET_DCDMMIS(v)             BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTMIS_GET_DSRMMIS(v)             BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTMIS_GET_RXMIS(v)               BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTMIS_GET_TXMIS(v)               BF_EXTRACT(v, 5 , 1 )
#define PL011_UARTMIS_GET_RTMIS(v)               BF_EXTRACT(v, 6 , 1 )
#define PL011_UARTMIS_GET_FEMIS(v)               BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTMIS_GET_PEMIS(v)               BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTMIS_GET_BEMIS(v)               BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTMIS_GET_OEMIS(v)               BF_EXTRACT(v, 10, 1 )
#define PL011_UARTMIS_CLR_SET_RIMMIS(v, set)             BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTMIS_CLR_SET_CTSMMIS(v, set)            BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTMIS_CLR_SET_DCDMMIS(v, set)            BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTMIS_CLR_SET_DSRMMIS(v, set)            BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTMIS_CLR_SET_RXMIS(v, set)              BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTMIS_CLR_SET_TXMIS(v, set)              BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define PL011_UARTMIS_CLR_SET_RTMIS(v, set)              BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define PL011_UARTMIS_CLR_SET_FEMIS(v, set)              BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTMIS_CLR_SET_PEMIS(v, set)              BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTMIS_CLR_SET_BEMIS(v, set)              BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTMIS_CLR_SET_OEMIS(v, set)              BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTMIS_CLR_RIMMIS(v)              BF_CLEAR(v, 0 , 1 )
#define PL011_UARTMIS_CLR_CTSMMIS(v)             BF_CLEAR(v, 1 , 1 )
#define PL011_UARTMIS_CLR_DCDMMIS(v)             BF_CLEAR(v, 2 , 1 )
#define PL011_UARTMIS_CLR_DSRMMIS(v)             BF_CLEAR(v, 3 , 1 )
#define PL011_UARTMIS_CLR_RXMIS(v)               BF_CLEAR(v, 4 , 1 )
#define PL011_UARTMIS_CLR_TXMIS(v)               BF_CLEAR(v, 5 , 1 )
#define PL011_UARTMIS_CLR_RTMIS(v)               BF_CLEAR(v, 6 , 1 )
#define PL011_UARTMIS_CLR_FEMIS(v)               BF_CLEAR(v, 7 , 1 )
#define PL011_UARTMIS_CLR_PEMIS(v)               BF_CLEAR(v, 8 , 1 )
#define PL011_UARTMIS_CLR_BEMIS(v)               BF_CLEAR(v, 9 , 1 )
#define PL011_UARTMIS_CLR_OEMIS(v)               BF_CLEAR(v, 10, 1 )

static inline int pl011_uartmis_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTMIS_GET_RIMMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRIMMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_CTSMMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTSMMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_DCDMMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCDMMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_DSRMMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDSRMMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_RXMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_TXMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_RTMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_FEMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sFEMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_PEMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sPEMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_BEMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBEMIS", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTMIS_GET_OEMIS(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOEMIS", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uartmis_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RIMMIS:%x,CTSMMIS:%x,DCDMMIS:%x,DSRMMIS:%x,RXMIS:%x,TXMIS:%x,RTMIS:%x,FEMIS:%x,PEMIS:%x,BEMIS:%x,OEMIS:%x",
    v,
    (int)PL011_UARTMIS_GET_RIMMIS(v),
    (int)PL011_UARTMIS_GET_CTSMMIS(v),
    (int)PL011_UARTMIS_GET_DCDMMIS(v),
    (int)PL011_UARTMIS_GET_DSRMMIS(v),
    (int)PL011_UARTMIS_GET_RXMIS(v),
    (int)PL011_UARTMIS_GET_TXMIS(v),
    (int)PL011_UARTMIS_GET_RTMIS(v),
    (int)PL011_UARTMIS_GET_FEMIS(v),
    (int)PL011_UARTMIS_GET_PEMIS(v),
    (int)PL011_UARTMIS_GET_BEMIS(v),
    (int)PL011_UARTMIS_GET_OEMIS(v));
}
#define PL011_UARTICR_GET_RIMIC(v)               BF_EXTRACT(v, 0 , 1 )
#define PL011_UARTICR_GET_CTSMIC(v)              BF_EXTRACT(v, 1 , 1 )
#define PL011_UARTICR_GET_DCDMIC(v)              BF_EXTRACT(v, 2 , 1 )
#define PL011_UARTICR_GET_DSRMIC(v)              BF_EXTRACT(v, 3 , 1 )
#define PL011_UARTICR_GET_RXIC(v)                BF_EXTRACT(v, 4 , 1 )
#define PL011_UARTICR_GET_TXIC(v)                BF_EXTRACT(v, 5 , 1 )
#define PL011_UARTICR_GET_RTIC(v)                BF_EXTRACT(v, 6 , 1 )
#define PL011_UARTICR_GET_FEIC(v)                BF_EXTRACT(v, 7 , 1 )
#define PL011_UARTICR_GET_PEIC(v)                BF_EXTRACT(v, 8 , 1 )
#define PL011_UARTICR_GET_BEIC(v)                BF_EXTRACT(v, 9 , 1 )
#define PL011_UARTICR_GET_OEIC(v)                BF_EXTRACT(v, 10, 1 )
#define PL011_UARTICR_CLR_SET_RIMIC(v, set)              BF_CLEAR_AND_SET(v, set, 0 , 1 )
#define PL011_UARTICR_CLR_SET_CTSMIC(v, set)             BF_CLEAR_AND_SET(v, set, 1 , 1 )
#define PL011_UARTICR_CLR_SET_DCDMIC(v, set)             BF_CLEAR_AND_SET(v, set, 2 , 1 )
#define PL011_UARTICR_CLR_SET_DSRMIC(v, set)             BF_CLEAR_AND_SET(v, set, 3 , 1 )
#define PL011_UARTICR_CLR_SET_RXIC(v, set)               BF_CLEAR_AND_SET(v, set, 4 , 1 )
#define PL011_UARTICR_CLR_SET_TXIC(v, set)               BF_CLEAR_AND_SET(v, set, 5 , 1 )
#define PL011_UARTICR_CLR_SET_RTIC(v, set)               BF_CLEAR_AND_SET(v, set, 6 , 1 )
#define PL011_UARTICR_CLR_SET_FEIC(v, set)               BF_CLEAR_AND_SET(v, set, 7 , 1 )
#define PL011_UARTICR_CLR_SET_PEIC(v, set)               BF_CLEAR_AND_SET(v, set, 8 , 1 )
#define PL011_UARTICR_CLR_SET_BEIC(v, set)               BF_CLEAR_AND_SET(v, set, 9 , 1 )
#define PL011_UARTICR_CLR_SET_OEIC(v, set)               BF_CLEAR_AND_SET(v, set, 10, 1 )
#define PL011_UARTICR_CLR_RIMIC(v)               BF_CLEAR(v, 0 , 1 )
#define PL011_UARTICR_CLR_CTSMIC(v)              BF_CLEAR(v, 1 , 1 )
#define PL011_UARTICR_CLR_DCDMIC(v)              BF_CLEAR(v, 2 , 1 )
#define PL011_UARTICR_CLR_DSRMIC(v)              BF_CLEAR(v, 3 , 1 )
#define PL011_UARTICR_CLR_RXIC(v)                BF_CLEAR(v, 4 , 1 )
#define PL011_UARTICR_CLR_TXIC(v)                BF_CLEAR(v, 5 , 1 )
#define PL011_UARTICR_CLR_RTIC(v)                BF_CLEAR(v, 6 , 1 )
#define PL011_UARTICR_CLR_FEIC(v)                BF_CLEAR(v, 7 , 1 )
#define PL011_UARTICR_CLR_PEIC(v)                BF_CLEAR(v, 8 , 1 )
#define PL011_UARTICR_CLR_BEIC(v)                BF_CLEAR(v, 9 , 1 )
#define PL011_UARTICR_CLR_OEIC(v)                BF_CLEAR(v, 10, 1 )

static inline int pl011_uarticr_bitmask_to_string(char *buf, int bufsz, uint32_t v)
{
  int n = 0;
  int first = 1;
  if (PL011_UARTICR_GET_RIMIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRIMIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_CTSMIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sCTSMIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_DCDMIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDCDMIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_DSRMIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sDSRMIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_RXIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRXIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_TXIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sTXIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_RTIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sRTIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_FEIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sFEIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_PEIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sPEIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_BEIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sBEIC", first ? "" : ",");
    first = 0;
  }
  if (PL011_UARTICR_GET_OEIC(v)) {
    n += snprintf(buf + n, bufsz - n, "%sOEIC", first ? "" : ",");
    first = 0;
  }
  return n;
}


static inline int pl011_uarticr_to_string(char *buf, int bufsz, uint32_t v)
{
  return snprintf(buf, bufsz, "%08x,RIMIC:%x,CTSMIC:%x,DCDMIC:%x,DSRMIC:%x,RXIC:%x,TXIC:%x,RTIC:%x,FEIC:%x,PEIC:%x,BEIC:%x,OEIC:%x",
    v,
    (int)PL011_UARTICR_GET_RIMIC(v),
    (int)PL011_UARTICR_GET_CTSMIC(v),
    (int)PL011_UARTICR_GET_DCDMIC(v),
    (int)PL011_UARTICR_GET_DSRMIC(v),
    (int)PL011_UARTICR_GET_RXIC(v),
    (int)PL011_UARTICR_GET_TXIC(v),
    (int)PL011_UARTICR_GET_RTIC(v),
    (int)PL011_UARTICR_GET_FEIC(v),
    (int)PL011_UARTICR_GET_PEIC(v),
    (int)PL011_UARTICR_GET_BEIC(v),
    (int)PL011_UARTICR_GET_OEIC(v));
}
