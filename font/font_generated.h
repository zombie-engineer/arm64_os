#include <font.h>
static ALIGNED(64) char font_bitmap_raw_myfont[] = {
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x0e, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x11, 0x03, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x0a, 0x02, 0x0b, 0x01, 0x00, 0x02, 0x01, 0x04, 0x04, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x01, 0x00, 0x1f, 0x0c, 0x04, 0x02, 0x00, 0x01, 0x02, 0x0e, 0x04, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x01, 0x00, 0x0a, 0x11, 0x02, 0x1f, 0x00, 0x01, 0x02, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x05, 0x1f, 0x0e, 0x0d, 0x09, 0x01, 0x01, 0x02, 0x0e, 0x04, 0x02, 0x00, 0x00, 0x02,
    0x00, 0x01, 0x05, 0x0a, 0x04, 0x0c, 0x16, 0x01, 0x02, 0x01, 0x04, 0x04, 0x01, 0x0f, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x04, 0x0f, 0x0f, 0x09, 0x0f, 0x0e, 0x1f, 0x0e, 0x0e, 0x00, 0x00, 0x08, 0x00, 0x01, 0x0e,
    0x19, 0x06, 0x08, 0x10, 0x09, 0x01, 0x11, 0x10, 0x11, 0x11, 0x00, 0x00, 0x04, 0x00, 0x02, 0x11,
    0x19, 0x05, 0x08, 0x10, 0x09, 0x01, 0x01, 0x10, 0x11, 0x11, 0x00, 0x02, 0x02, 0x00, 0x04, 0x11,
    0x15, 0x04, 0x0f, 0x0e, 0x0f, 0x0f, 0x0f, 0x08, 0x0e, 0x1e, 0x01, 0x00, 0x01, 0x1f, 0x08, 0x10,
    0x15, 0x04, 0x01, 0x10, 0x08, 0x08, 0x11, 0x04, 0x11, 0x10, 0x00, 0x00, 0x02, 0x00, 0x04, 0x0c,
    0x13, 0x04, 0x01, 0x10, 0x08, 0x08, 0x11, 0x02, 0x11, 0x11, 0x00, 0x02, 0x04, 0x00, 0x02, 0x00,
    0x0e, 0x04, 0x0f, 0x0f, 0x08, 0x0f, 0x0e, 0x02, 0x0e, 0x0e, 0x01, 0x01, 0x08, 0x1f, 0x01, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x04, 0x0f, 0x0e, 0x0f, 0x1f, 0x1f, 0x0e, 0x11, 0x07, 0x1f, 0x11, 0x01, 0x41, 0x21, 0x0e,
    0x19, 0x04, 0x11, 0x11, 0x11, 0x01, 0x01, 0x11, 0x11, 0x02, 0x10, 0x09, 0x01, 0x63, 0x23, 0x11,
    0x15, 0x0a, 0x11, 0x01, 0x11, 0x01, 0x01, 0x01, 0x11, 0x02, 0x10, 0x05, 0x01, 0x55, 0x25, 0x11,
    0x0d, 0x0a, 0x0d, 0x01, 0x11, 0x0f, 0x0f, 0x01, 0x1f, 0x02, 0x10, 0x03, 0x01, 0x49, 0x29, 0x11,
    0x05, 0x0e, 0x11, 0x01, 0x11, 0x01, 0x01, 0x19, 0x11, 0x02, 0x10, 0x05, 0x01, 0x41, 0x31, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x01, 0x01, 0x11, 0x11, 0x02, 0x11, 0x09, 0x01, 0x41, 0x21, 0x11,
    0x0e, 0x11, 0x0f, 0x0e, 0x0f, 0x1f, 0x01, 0x0e, 0x11, 0x07, 0x0e, 0x11, 0x1f, 0x41, 0x21, 0x0e,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0f, 0x0e, 0x0f, 0x0e, 0x1f, 0x11, 0x11, 0x41, 0x22, 0x22, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x11, 0x11, 0x11, 0x04, 0x11, 0x11, 0x41, 0x22, 0x22, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x11, 0x11, 0x01, 0x04, 0x11, 0x0a, 0x63, 0x14, 0x14, 0x10, 0x03, 0x01, 0x03, 0x00, 0x00,
    0x0f, 0x11, 0x0d, 0x0f, 0x04, 0x11, 0x0a, 0x2a, 0x08, 0x1c, 0x08, 0x01, 0x02, 0x02, 0x00, 0x00,
    0x01, 0x19, 0x05, 0x10, 0x04, 0x11, 0x0a, 0x2a, 0x14, 0x08, 0x04, 0x01, 0x04, 0x02, 0x00, 0x00,
    0x01, 0x19, 0x09, 0x11, 0x04, 0x11, 0x04, 0x36, 0x22, 0x08, 0x02, 0x01, 0x08, 0x02, 0x02, 0x00,
    0x01, 0x2e, 0x11, 0x0e, 0x04, 0x0e, 0x04, 0x14, 0x22, 0x08, 0x3e, 0x03, 0x10, 0x03, 0x05, 0x3f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x0e, 0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x01, 0x00, 0x08, 0x00, 0x0e, 0x09, 0x01, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x07, 0x06, 0x0e, 0x0f, 0x01, 0x09, 0x01, 0x01, 0x04, 0x05, 0x01, 0x0b, 0x07, 0x06,
    0x00, 0x0e, 0x09, 0x09, 0x09, 0x01, 0x07, 0x0e, 0x07, 0x01, 0x04, 0x03, 0x01, 0x15, 0x09, 0x09,
    0x00, 0x09, 0x09, 0x01, 0x09, 0x0f, 0x01, 0x08, 0x09, 0x01, 0x04, 0x05, 0x01, 0x11, 0x09, 0x09,
    0x01, 0x09, 0x09, 0x09, 0x09, 0x01, 0x01, 0x0b, 0x09, 0x01, 0x05, 0x09, 0x09, 0x11, 0x09, 0x09,
    0x02, 0x1e, 0x07, 0x06, 0x0e, 0x0f, 0x01, 0x07, 0x09, 0x01, 0x02, 0x09, 0x06, 0x11, 0x09, 0x06,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00,
    0x07, 0x0e, 0x00, 0x0e, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x09, 0x0d, 0x01, 0x02, 0x09, 0x11, 0x11, 0x09, 0x09, 0x0f, 0x02, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x09, 0x0b, 0x06, 0x07, 0x09, 0x1b, 0x11, 0x09, 0x09, 0x08, 0x01, 0x01, 0x04, 0x00, 0x00,
    0x07, 0x0e, 0x01, 0x08, 0x02, 0x09, 0x0a, 0x15, 0x06, 0x0e, 0x06, 0x02, 0x01, 0x02, 0x00, 0x00,
    0x01, 0x08, 0x01, 0x09, 0x0a, 0x09, 0x0e, 0x15, 0x09, 0x08, 0x01, 0x02, 0x01, 0x02, 0x0d, 0x00,
    0x01, 0x08, 0x01, 0x06, 0x06, 0x06, 0x04, 0x0a, 0x09, 0x0e, 0x0f, 0x04, 0x01, 0x01, 0x12, 0x00
};

#define DECL_GLYPH(_px, _py, _bbx, _bby, _brx, _bry, _ax, _ay) \
{\
  .pos_x = _px,\
  .pos_y = _py,\
  .bound_x = _bbx,\
  .bound_y = _bby,\
  .bearing_x = _brx,\
  .bearing_y = _bry,\
  .advance_x = _ax,\
  .advance_y = _ay\
}

static ALIGNED(64) font_glyph_metrics_t font_glyph_metrics_myfont[] = {
  DECL_GLYPH(  0,  7, /* bnd box */ 5, 8, /* bearing */  1,  0, /* adv */ 1, 0), // ' '
  DECL_GLYPH(  8,  7, /* bnd box */ 1, 7, /* bearing */  2,  0, /* adv */ 2, 0), // !
  DECL_GLYPH( 16,  7, /* bnd box */ 3, 2, /* bearing */  2,  3, /* adv */ 1, 0), // "
  DECL_GLYPH( 24,  7, /* bnd box */ 5, 5, /* bearing */  1,  2, /* adv */ 2, 0), // #
  DECL_GLYPH( 32,  7, /* bnd box */ 5, 8, /* bearing */  1,  0, /* adv */ 2, 0), // $
  DECL_GLYPH( 40,  7, /* bnd box */ 4, 6, /* bearing */  1,  1, /* adv */ 1, 0), // %
  DECL_GLYPH( 48,  7, /* bnd box */ 7, 5, /* bearing */  1,  0, /* adv */ 1, 0), // &
  DECL_GLYPH( 56,  7, /* bnd box */ 1, 2, /* bearing */  2,  4, /* adv */ 1, 0), // '
  DECL_GLYPH( 64,  7, /* bnd box */ 2, 6, /* bearing */  2,  1, /* adv */ 1, 0), // (
  DECL_GLYPH( 72,  7, /* bnd box */ 2, 6, /* bearing */  1,  1, /* adv */ 2, 0), // )
  DECL_GLYPH( 80,  7, /* bnd box */ 5, 5, /* bearing */  1,  2, /* adv */ 2, 0), // *
  DECL_GLYPH( 88,  7, /* bnd box */ 5, 5, /* bearing */  1,  2, /* adv */ 2, 0), // +
  DECL_GLYPH( 96,  7, /* bnd box */ 2, 2, /* bearing */  1, -1, /* adv */ 2, 0), // ,
  DECL_GLYPH(104,  7, /* bnd box */ 4, 1, /* bearing */  1,  3, /* adv */ 1, 0), // -
  DECL_GLYPH(112,  7, /* bnd box */ 1, 1, /* bearing */  1,  0, /* adv */ 1, 0), // .
  DECL_GLYPH(120,  7, /* bnd box */ 5, 5, /* bearing */  1,  0, /* adv */ 1, 0), // /

  DECL_GLYPH(  0, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 0
  DECL_GLYPH(  8, 15, /* bnd box */ 3, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 1
  DECL_GLYPH( 16, 15, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 2
  DECL_GLYPH( 24, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 3
  DECL_GLYPH( 32, 15, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 4
  DECL_GLYPH( 40, 15, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 5
  DECL_GLYPH( 48, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 6
  DECL_GLYPH( 56, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 7
  DECL_GLYPH( 64, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // 8
  DECL_GLYPH( 72, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 2, 0), // 9
  DECL_GLYPH( 80, 15, /* bnd box */ 1, 4, /* bearing */  2,  2, /* adv */ 2, 0), // :
  DECL_GLYPH( 88, 15, /* bnd box */ 2, 5, /* bearing */  1,  1, /* adv */ 2, 0), // ;
  DECL_GLYPH( 96, 15, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // <
  DECL_GLYPH(104, 15, /* bnd box */ 5, 4, /* bearing */  2,  2, /* adv */ 2, 0), // =
  DECL_GLYPH(112, 15, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // >
  DECL_GLYPH(120, 15, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // ?

  DECL_GLYPH(  0, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // @
  DECL_GLYPH(  8, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // A
  DECL_GLYPH( 16, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // B
  DECL_GLYPH( 24, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // C
  DECL_GLYPH( 32, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // D
  DECL_GLYPH( 40, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // E
  DECL_GLYPH( 48, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // F
  DECL_GLYPH( 56, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // G
  DECL_GLYPH( 64, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // H
  DECL_GLYPH( 72, 23, /* bnd box */ 3, 7, /* bearing */  2,  0, /* adv */ 2, 0), // I
  DECL_GLYPH( 80, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // J
  DECL_GLYPH( 88, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // K
  DECL_GLYPH( 96, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // L
  DECL_GLYPH(104, 23, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // M
  DECL_GLYPH(112, 23, /* bnd box */ 6, 7, /* bearing */  1,  0, /* adv */ 1, 0), // N
  DECL_GLYPH(120, 23, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // O

  DECL_GLYPH(  0, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // P
  DECL_GLYPH(  8, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // Q
  DECL_GLYPH( 16, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // R
  DECL_GLYPH( 24, 31, /* bnd box */ 5, 7, /* bearing */  1,  0, /* adv */ 1, 0), // S
  DECL_GLYPH( 32, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // T
  DECL_GLYPH( 40, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // U
  DECL_GLYPH( 48, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // V
  DECL_GLYPH( 56, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // W
  DECL_GLYPH( 64, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // X
  DECL_GLYPH( 72, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // Y
  DECL_GLYPH( 80, 31, /* bnd box */ 7, 7, /* bearing */  1,  0, /* adv */ 1, 0), // Z
  DECL_GLYPH( 88, 31, /* bnd box */ 2, 5, /* bearing */  1,  1, /* adv */ 1, 0), // [
  DECL_GLYPH( 96, 31, /* bnd box */ 5, 5, /* bearing */  1,  0, /* adv */ 1, 0), // '\'
  DECL_GLYPH(104, 31, /* bnd box */ 2, 5, /* bearing */  1,  1, /* adv */ 1, 0), // ]
  DECL_GLYPH(112, 31, /* bnd box */ 7, 2, /* bearing */  1,  4, /* adv */ 1, 0), // ^
  DECL_GLYPH(120, 31, /* bnd box */ 6, 1, /* bearing */  1,  0, /* adv */ 1, 0), // _

  DECL_GLYPH(  0, 39, /* bnd box */ 2, 2, /* bearing */  1,  4, /* adv */ 1, 0), // `
  DECL_GLYPH(  8, 39, /* bnd box */ 5, 6, /* bearing */  1,  0, /* adv */ 1, 0), // a
  DECL_GLYPH( 16, 39, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // b
  DECL_GLYPH( 24, 39, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // c
  DECL_GLYPH( 32, 39, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // d
  DECL_GLYPH( 40, 39, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // e
  DECL_GLYPH( 48, 39, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // f
  DECL_GLYPH( 56, 39, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // g
  DECL_GLYPH( 64, 39, /* bnd box */ 4, 7, /* bearing */  1,  0, /* adv */ 1, 0), // h
  DECL_GLYPH( 72, 39, /* bnd box */ 1, 7, /* bearing */  1,  0, /* adv */ 1, 0), // i
  DECL_GLYPH( 80, 39, /* bnd box */ 3, 7, /* bearing */  1,  0, /* adv */ 1, 0), // j
  DECL_GLYPH( 88, 39, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // k
  DECL_GLYPH( 96, 39, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // l
  DECL_GLYPH(104, 39, /* bnd box */ 5, 5, /* bearing */  1,  0, /* adv */ 1, 0), // m
  DECL_GLYPH(112, 39, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // n
  DECL_GLYPH(120, 39, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // o

  DECL_GLYPH(  0, 47, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 0, 0), // p
  DECL_GLYPH(  8, 47, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // q
  DECL_GLYPH( 16, 47, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // r
  DECL_GLYPH( 24, 47, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // s
  DECL_GLYPH( 32, 47, /* bnd box */ 4, 6, /* bearing */  1,  0, /* adv */ 1, 0), // t
  DECL_GLYPH( 40, 47, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // u
  DECL_GLYPH( 48, 47, /* bnd box */ 5, 5, /* bearing */  1,  0, /* adv */ 1, 0), // v
  DECL_GLYPH( 56, 47, /* bnd box */ 5, 5, /* bearing */  1,  0, /* adv */ 1, 0), // w
  DECL_GLYPH( 64, 47, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // x
  DECL_GLYPH( 72, 47, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // y
  DECL_GLYPH( 80, 47, /* bnd box */ 4, 5, /* bearing */  1,  0, /* adv */ 1, 0), // z
  DECL_GLYPH( 88, 47, /* bnd box */ 3, 7, /* bearing */  1,  0, /* adv */ 1, 0), // {
  DECL_GLYPH( 96, 47, /* bnd box */ 1, 7, /* bearing */  1,  0, /* adv */ 1, 0), // |
  DECL_GLYPH(104, 47, /* bnd box */ 3, 7, /* bearing */  1,  0, /* adv */ 1, 0), // }
  DECL_GLYPH(112, 47, /* bnd box */ 5, 2, /* bearing */  1,  0, /* adv */ 1, 0)  // ~
};
