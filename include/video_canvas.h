#pragma once

void video_canvas_init(int width, int height);

void video_canvas_set_bgcolor(int value);

int video_canvas_is_initialized();

void video_canvas_showpicture();

// video console method. put single char on screen
void video_canvas_putc(int *x, int *y, char);

// video console method. put string on screen
void video_canvas_puts(int *x, int *y, const char *s);

int video_canvas_get_width_height(int *width, int *height);

