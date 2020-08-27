//#include "config.h"
#include "text.h"
//#include "display.h"

void draw_text(uint16_t *fb, int fb_width, int fb_stride, int fb_height, const char *text, int x, int y, uint16_t color, uint16_t bg_color)
{
#ifdef DISPLAY_FLIP_ORIENTATION_IN_SOFTWARE
  const int W = fb_height;
  const int H = fb_width;
#define AT(x, y) x*fb_stride+y
#else
  const int W = fb_width;
  const int H = fb_height;
#define AT(x, y) y*fb_stride+x
#endif

  fb_stride >>= 1; // to uint16 elements
  const int Y = y;
  while(*text)
  {
    uint8_t ch = (uint8_t)*text;
    if (ch < 32 || ch >= 127) ch = 0;
    else ch -= 32;

    const int X = x;
    const int endX = x + MONACO_WIDTH;

    for(y = Y - 1; y < Y + monaco_height_adjust[ch]; ++y) {
      for(int x = X; x < endX+1; ++x) {
        if (x >= 0 && y >= 0 && x < W && y < H)
          fb[AT(x,y)] = bg_color;
      }
    }

    y = Y + monaco_height_adjust[ch];
    int yEnd = Y + MONACO_HEIGHT - 1;

    const uint8_t *byte = monaco_font + ch*MONACO_BYTES_PER_CHAR;
    for(int i = 0; i < MONACO_BYTES_PER_CHAR; ++i, ++byte) {
      for(uint8_t bit = 1; bit; bit <<= 1) {
        if (x >= 0 && y >= 0 && x < W && y < H) {
          if ((*byte & bit)) 
            fb[AT(x,y)] = color;
          else 
            fb[AT(x,y)] = bg_color;
        }
        ++x;
        if (x == endX)
        {
          if (y < H) 
            fb[AT(x,y)] = bg_color;
          x = X;
          ++y;
          if (y == yEnd)
          {
            i = MONACO_BYTES_PER_CHAR;
            bit = 0;
            break;
          }
        }
      }
    }
    ++text;
    x += 6;
  }
}
