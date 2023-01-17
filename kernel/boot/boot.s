
	.text
	.code64
	.globl	startup
	.globl	spin
	.globl	cpu_relax

	.globl	inb
	.globl	inw
	.globl	inl
	.globl	outb
	.globl	outw
	.globl	outl

	.globl	__framebuffer
	.globl	__framebuffer_size
	.globl	__horizontal_pixels
	.globl	__vertical_pixels
	.globl	__pixels_per_line

# filled in by the loader
__framebuffer:
	.quad	0
__framebuffer_size:
	.quad	0
__horizontal_pixels:
	.quad	0
__vertical_pixels:
	.quad	0
__pixels_per_line:
	.quad	0


startup:
	call	kernel_entry

inb:
	movw	%di, %dx
	inb	%dx, %al
	ret
inw:
	movw	%di, %dx
	inw	%dx, %ax
	ret
inl:
	movw	%di, %dx
	inl	%dx, %eax
	ret
outb:
	movw	%di, %dx
	movb	%sil, %al
	outb	%al, %dx
	ret

outw:
	movw	%di, %dx
	movw	%si, %ax
	outw	%ax, %dx
	ret

outl:
	movw	%di, %dx
	movl	%esi, %eax
	outl	%eax, %dx
	ret

spin:
	movq	%rdi, %rcx
	shl	$20, %rcx
1:
	decq	%rcx
	pause
	jnz	1b
	ret

cpu_relax:
	pause
	jmp	cpu_relax
