#pragma once

void lfb_init(int width, int height);

int lfb_is_initialized();

void lfb_showpicture();

// video console method. put single char on screen
void lfb_putc(int *x, int *y, char);

// video console method. put string on screen
void lfb_puts(int *x, int *y, const char *s);

int lfb_get_width_height(int *width, int *height);

