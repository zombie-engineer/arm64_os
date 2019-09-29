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

void viewport_fill_rect(viewport_t *v, int x, int y, unsigned int size_x, unsigned int size_y, int rgba);

void viewport_draw_text(viewport_t *v, int x, int y, int text_color, int bg_color, const char* text, int textlen);
