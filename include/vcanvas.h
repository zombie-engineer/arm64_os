#pragma once

void vcanvas_init(int width, int height);

void vcanvas_set_fg_color(int value);

void vcanvas_set_bg_color(int value);

int vcanvas_is_initialized();

void vcanvas_showpicture();

// video console method. put single char on screen
void vcanvas_putc(int *x, int *y, char);

// video console method. put string on screen
void vcanvas_puts(int *x, int *y, const char *s);

int vcanvas_get_width_height(int *width, int *height);

void vcanvas_fill_rect(int x, int y, unsigned int size_x, unsigned int size_y, int rgba);

typedef struct viewport {
  int pos_x;
  int pos_y;
  unsigned int size_x;
  unsigned int size_y;
} viewport_t;

viewport_t * vcanvas_make_viewport(int x, int y, unsigned int size_x, unsigned int size_y);

/* fills whole viewport with given color */
void viewport_fill(viewport_t *v, int color);

/* fills rectangle in viewport with given color 
 * x, y - rectangle starting position, given in coordinates,
 * relative to viewport 'v'. 
 * size_x, size_y - rectangle with and height
 * color - rectangle color
 */
void viewport_fill_rect(viewport_t *v, int x, int y, unsigned int size_x, unsigned int size_y, int color);

/* draws line of text in viewport.
 * coordinates x and y are relative to this viewport
 */
void viewport_draw_text(viewport_t *v, int x0, int y, int fg_color, int bg_color, const char* text, int textlen);

/* draws a single char in viewport.
 * coordinates x and y are relative to this viewport
 */
void viewport_draw_char(viewport_t *v, int x, int y, int fg_color, int bg_color, char);

int vcanvas_get_fontsize(int *size_x, int *size_y);

/* copyes rectangle from position [x0:y0, x0+size_x:y0+size_y] 
 * to [x1:y1, x1+size_x:y2+sizey]
 */
void viewport_copy_rect(viewport_t *v, int x, int y0, int size_x, int size_y, int x1, int y1);
