#include <stdint.h>
#include <framebuffer.h>

typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} psf_font;

/* font data */
extern char _binary_font_uni2_bold_18_10_psf_start;
extern char _binary_font_uni2_bold_18_10_psf_end;
extern char _binary_font_uni2_bold_32_16_psf_start;
extern char _binary_font_uni2_bold_32_16_psf_end;
extern char _binary_font_lat2_bold_32_16_psf_start;
extern char _binary_font_lat2_bold_32_16_psf_end;

static uint16_t uni2_18_10[UINT16_MAX];
static uint16_t uni2_32_16[UINT16_MAX];
static uint16_t lat2_32_16[UINT16_MAX];
static psf_font *uni2_18_10_font;
static psf_font *uni2_32_16_font;
static psf_font *lat2_32_16_font;

/* console status */
static uint32_t fg = 0x3de1ad;
static uint32_t bg = 0x111166;
static uint32_t pos_x = 0;
static uint32_t pos_y = 0;
static uint16_t *font;
static psf_font *font_desc;

extern void spin(uint64_t t);

/* 
 * the frame buffer
 * once we have malloc, this should be changed
 */
static uint32_t *(front_buffer[2160]);
static uint32_t back_buffer[2160][3840];

static inline void
swap_buffer(uint32_t x, uint32_t width, uint32_t y, uint32_t height)
{
	uint32_t i, j;

	for (j = y; j < y + height; ++j) {
		for (i = x; i < x + width; ++i) {
			front_buffer[j][i] = back_buffer[j][i];
		}
	}
}

static void
scroll_down()
{
	uint32_t i, j, last, height;

	height = font_desc->height;
	last = (_frameb_height / height) * height;
	for (j = 0; j < last - height; ++j) {
		for (i = 0; i < _frameb_width; ++i) {
			back_buffer[j][i] = back_buffer[j + height][i];
		}
	}
	for (; j < last; ++j) {
		for (i = 0; i < _frameb_width; ++i) {
			back_buffer[j][i] = bg;
		}
	}

	swap_buffer(0, _frameb_width, 0, last);
}

static inline void con_next_pos_x()
{
	if (pos_x + font_desc->width <= _frameb_width - font_desc->width) {
		pos_x += font_desc->width;
	} else {
		pos_x = 0;
	}
}

static inline void con_next_pos_y()
{
	if (pos_y + font_desc->height <= _frameb_height - font_desc->height) {
		pos_y += font_desc->height;
	} else {
		scroll_down();
	}
}

static inline void con_next_pos()
{
	if (pos_x + font_desc->width <= _frameb_width - font_desc->width) {
		pos_x += font_desc->width;
	} else {
		pos_x = 0;
		con_next_pos_y();
	}
}

void con_putchar(char c)
{
	int bytes_per_line;
	uint16_t unicode;
	uint8_t *glyph;
	uint32_t x, y, mask;
	uint32_t *current_addr;

	if (c == '\n') {
		pos_x = 0;
		con_next_pos_y();
		return;
	}

	bytes_per_line = (font_desc->width + 7) / 8;
	unicode = font[c];
	glyph = (uint8_t *) font_desc + font_desc->headersize +
		(unicode > 0 && unicode < font_desc->numglyph ? unicode : 0) *
		font_desc->bytesperglyph;

	for (y = 0; y < font_desc->height; ++y) {
		mask = 1 << (font_desc->width - 1);
		uint16_t tmp = *((uint16_t *) glyph);
		uint16_t flip = (tmp >> 8) | (tmp << 8);
		flip >>= (16 - font_desc->width);
		for (x = 0; x < font_desc->width; ++x) {
			back_buffer[pos_y + y][pos_x + x] = flip & mask ? fg : bg;
			mask >>= 1;
		}
		glyph += bytes_per_line;
	}
	swap_buffer(pos_x, font_desc->width, pos_y, font_desc->height);
	con_next_pos();
}

void con_puts(char *s)
{
	while (*s) {
		con_putchar(*(s++));
	}
}

void con_clear_screen()
{
	uint64_t x, y, line_pos;

	for (y = 0; y < _frameb_height; ++y) {
		for (x = 0; x < _frameb_width; ++x) {
			back_buffer[y][x] = bg;
		}
	}
	swap_buffer(0, _frameb_width, 0, _frameb_height);
}

void con_set_cursor(uint32_t x, uint32_t y)
{
	uint32_t rightmost, downmost, height, width;

	height = font_desc->height;
	width = font_desc->width;
	rightmost = _frameb_width / width * width - width;
	downmost = _frameb_height / height * height - height;

	pos_x = x * width > rightmost ? rightmost : x * width;
	pos_y = y * height > downmost ? downmost : y * height;
}

void con_set_font(uint32_t height, uint32_t width)
{
	if (height < 25) {
		font = uni2_18_10;
		font_desc = uni2_18_10_font;
	} else {
		font = uni2_32_16;
		font_desc = uni2_32_16_font;
	}
	pos_x = pos_y = 0;
	con_clear_screen();
}

void con_set_color(uint32_t _bg, uint32_t _fg)
{
	bg = _bg;
	fg = _fg;
}

void con_get_font(uint32_t *height, uint32_t *width)
{
	*height = font_desc->height;
	*width = font_desc->width;
}

void con_get_color(uint32_t *_bg, uint32_t *_fg)
{
	*_bg = bg;
	*_fg = fg;
}

static void
_psf_init(uint64_t start, uint64_t end, uint16_t *table, psf_font **font_head)
{
	uint16_t glyph;
	uint8_t *t;
	psf_font * font;

	glyph = 0;
	*font_head = (psf_font *) start;
	font = *font_head;
	t = (uint8_t *) (start + font->headersize +
		font->numglyph * font->bytesperglyph);
	while (t < (uint8_t *) end) {
		uint16_t uc = (uint16_t) t[0];
		if (uc == 0xFF) {
			glyph++;
			t++;
			continue;
		} else if (uc & 128) {
			if ((uc & 32) == 0) {
				uc = ((t[0] & 0x1F) <<6 ) + (t[1] & 0x3F);
				t++;
			} else if ((uc & 16) == 0) {
				uc = ((((t[0] & 0xF) <<6 ) +
					(t[1] & 0x3F)) <<6 ) + (t[2] & 0x3F);
				t += 2;
			} else if ((uc & 8) == 0) {
				uc = ((((((t[0] & 0x7) <<6 ) + (t[1] & 0x3F)) << 6)
					+ (t[2] & 0x3F)) << 6) + (t[3] & 0x3F);
				t += 3;
			} else
				uc = 0;
		}
		table[uc] = glyph;
		t++;
	}
}

static void
frameb_init()
{
	int i;

	for (i = 0; i < _frameb_height; ++i) {
		front_buffer[i] = _frameb_base + i * _frameb_scanline;
	}
}

static void
psf_init()
{
	_psf_init((uint64_t) &_binary_font_uni2_bold_18_10_psf_start,
		  (uint64_t) &_binary_font_uni2_bold_18_10_psf_end,
		  uni2_18_10, &uni2_18_10_font);
	_psf_init((uint64_t) &_binary_font_uni2_bold_32_16_psf_start,
		  (uint64_t) &_binary_font_uni2_bold_32_16_psf_end,
		  uni2_32_16, &uni2_32_16_font);
	_psf_init((uint64_t) &_binary_font_lat2_bold_32_16_psf_start,
		  (uint64_t) &_binary_font_lat2_bold_32_16_psf_end,
		  lat2_32_16, &lat2_32_16_font);
	if (_frameb_width >= 3840) {
		font = uni2_32_16;
		font_desc = uni2_32_16_font;
	} else {
		font = uni2_18_10;
		font_desc = uni2_18_10_font;
	}
}

void con_init()
{
	frameb_init();
	psf_init();
}
