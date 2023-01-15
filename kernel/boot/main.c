
#include <stdint.h>
#include "font_16_32.h"

extern uint64_t framebuffer;
extern uint64_t framebuffer_size;
extern uint64_t horizontal_pixels;
extern uint64_t vertical_pixels;
extern uint64_t pixels_per_line;

static uint32_t *fbbase;
static uint64_t fbsize, width, height, line;

/* spin some amount of time proportional to t */
static void spin(uint64_t t)
{
	uint64_t i;

	for (i = 0; i < (t << 20); ++i);
}

static void spin_always()
{
	while (1);
}

static void fb_init()
{
	fbbase = (uint32_t *) (framebuffer);
	fbsize = framebuffer_size;
	width = horizontal_pixels;
	height = vertical_pixels;
	line = pixels_per_line;
}

static void clear_screen()
{
	uint32_t *current_addr;

	for (int v = 0; v < height; ++v) {
		current_addr = fbbase + line * v;
		for (int h = 0; h < width; ++h) {
			*(current_addr + h) = 0;
		}
	}
}

void kernel_entry()
{
	fb_init();
	clear_screen();

	uint32_t *current_addr;
	uint16_t *glyph;
	current_addr = fbbase;
	for (int i = 0; i < 256 * 8; ++i) {
		int horizontal = (i * 16) % width;
		int vertical = (i * 16) / width * 32;
		glyph = (uint16_t *) (i * 64 % (256 * 64) + font_16_32);
		for (int j = 0; j < 32; ++j) {
			uint16_t h = *(glyph + j);
			uint8_t low = h & 0xff;
			uint8_t high = (h >> 8) & 0xff;
			for (int k = 0; k < 8; ++k) {
				if (low & 0x80) {
					*(current_addr + (j + vertical) * line + k + horizontal) = 0x7bcfa6;
				}
				low <<= 1;
			}
			for (int k = 8; k < 16; ++k) {
				if (high & 0x80) {
					*(current_addr + (j + vertical) * line + k + horizontal) = 0x7bcfa6;
				}
				high <<= 1;
			}
		}
		spin(20);
	}
	spin_always();
}
