#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define PSF_FONT_MAGIC 0x864ab572

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

int main(int argc, char *argv[])
{
	char *psf_path, *psf;
	FILE *psf_file;
	off_t psf_size;
	psf_font *psf_font_head;
	int i;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <psu-file>\n", argv[0]);
		return 1;
	}

	psf_path = argv[1];
	psf_file = fopen(psf_path, "rb");
	if (psf_file == NULL) {
		fprintf(stderr, "Unable to open file: %s, error number: %d\n",
			psf_path, errno);
		return 1;
	}

	fseeko(psf_file, 0, SEEK_END);
	psf_size = ftello(psf_file);
	if (psf_size < 32) {
		fprintf(stderr, "psf file size (%ld) too small, at least 32\n" , psf_size);
		return 1;
	}

	psf = (char *) malloc(psf_size);
	if (!psf) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	fseeko(psf_file, 0, SEEK_SET);
	if (fread(psf, 1, psf_size, psf_file) != psf_size) {
		fprintf(stderr, "Cannot read %s, error number: %d,"
			" file size: %lu\n",
			psf_path, errno, psf_size);
		return 1;
	}

	psf_font_head = (psf_font *) psf;
	if (psf_font_head->magic != PSF_FONT_MAGIC) {
		fprintf(stderr, "Invalid psf file header: %x\n", psf_font_head->magic);
		return 1;
	}
	fprintf(stderr, "magic: %x\n", psf_font_head->magic);
	fprintf(stderr, "version: %u\n", psf_font_head->version);
	fprintf(stderr, "headersize: %u\n", psf_font_head->headersize);
	fprintf(stderr, "flags: %u\n", psf_font_head->flags);
	fprintf(stderr, "numglyph: %u\n", psf_font_head->numglyph);
	fprintf(stderr, "bytesperglyph: %u\n", psf_font_head->bytesperglyph);
	fprintf(stderr, "height: %u\n", psf_font_head->height);
	fprintf(stderr, "width: %u\n", psf_font_head->width);

	/* print as an array */
	fprintf(stdout, "\n{\n\t");
	for (i = psf_font_head->headersize; i < psf_font_head->headersize + psf_font_head->numglyph * psf_font_head->bytesperglyph; ++i) {
		if (i != psf_font_head->headersize && i % 16 == 0) {
			fprintf(stdout, "\n\t");
		}
		fprintf(stdout, "0x%.2x,%s", (unsigned char)*(psf + i), psf_font_head->headersize && i % 16 == 15 ? "" : " ");
	}
	fprintf(stdout, "\n}\n");
	return 0;
}
