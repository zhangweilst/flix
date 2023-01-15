
	.text
	.code64
	.globl	startup
	.globl	framebuffer
	.globl	framebuffer_size
	.globl	horizontal_pixels
	.globl	vertical_pixels
	.globl	pixels_per_line

# filled in by the loader
framebuffer:
	.quad	0
framebuffer_size:
	.quad	0
horizontal_pixels:
	.quad	0
vertical_pixels:
	.quad	0
pixels_per_line:
	.quad	0


startup:
	call	kernel_entry
