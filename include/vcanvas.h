#pragma once

void vcanvas_init(int width, int height);

void vcanvas_set_bgcolor(int value);

int vcanvas_is_initialized();

void vcanvas_showpicture();

// video console method. put single char on screen
void vcanvas_putc(int *x, int *y, char);

// video console method. put string on screen
void vcanvas_puts(int *x, int *y, const char *s);

int vcanvas_get_width_height(int *width, int *height);

void vcanvas_fill_rect(int x, int y, int width, int height, int rgba);
