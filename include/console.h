/**
 * The framebuffer based console interface
 * 
 */
#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>

extern void con_clear_screen();
extern void con_putchar(char c);
extern void con_puts(char *s);

/* set the cursor position */
extern void con_set_cursor(uint32_t x, uint32_t y);

/* set the font size */
extern void con_set_font(uint32_t height, uint32_t width);

/* set the console color with 0xrrggbb */
extern void con_set_color(uint32_t bg, uint32_t fg);

/* get the font size */
extern void con_get_font(uint32_t *height, uint32_t *width);

/* get the console color */
extern void con_get_color(uint32_t *bg, uint32_t *fg);

#endif
