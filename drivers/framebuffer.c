
#include <stdint.h>

extern uint64_t __framebuffer;
extern uint64_t __framebuffer_size;
extern uint64_t __horizontal_pixels;
extern uint64_t __vertical_pixels;
extern uint64_t __pixels_per_line;

uint32_t *_frameb_base;
uint64_t _frameb_size;
uint64_t _frameb_width;
uint64_t _frameb_height;
uint64_t _frameb_scanline;

void framebuffer_init()
{
	_frameb_base = (uint32_t *) (__framebuffer);
	_frameb_size = __framebuffer_size;
	_frameb_width = __horizontal_pixels;
	_frameb_height = __vertical_pixels;
	_frameb_scanline = __pixels_per_line;
}
