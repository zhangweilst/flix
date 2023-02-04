
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
	.globl	rdmsr
	.globl	wrmsr
	.globl	cpuid
	.globl	invtlb

	.globl	pre_mtrr_change
	.globl	post_mtrr_change

	.globl	rdcr0
	.globl	rdcr1
	.globl	rdcr2
	.globl	rdcr3
	.globl	rdcr4
	.globl	rdcr8

	.globl	__framebuffer
	.globl	__framebuffer_size
	.globl	__horizontal_pixels
	.globl	__vertical_pixels
	.globl	__pixels_per_line

	.globl	__acpi_version
	.globl	__acpi_table_rsdp

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
__acpi_version:
	.quad	0
__acpi_table_rsdp:
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

rdmsr:
	movl	%edi, %ecx
	rdmsr
	shl	$32, %rdx
	or	%rdx, %rax
	ret

wrmsr:
	movl	%edi, %ecx
	movq	%rsi, %r8
	shr	$32, %r8
	movq	%r8, %rdx
	movabs	$0xFFFFFFFF, %r8
	andq	%r8, %rsi
	movq	%rsi, %rax
	wrmsr
	ret

cpuid:
	pushq	%rbx
	movl	%edi, %eax
	movl	%esi, %ecx
	cpuid
	movabs	$0xFFFFFFFF, %r8
	shl	$32, %rbx
	andq	%r8, %rax
	orq	%rbx, %rax
	shl	$32, %rdx
	andq	%r8, %rcx
	orq	%rcx, %rdx
	popq	%rbx
	ret

rdcr0:
	movq	%cr0, %rax
	ret
rdcr1:
	movq	%cr1, %rax
	ret
rdcr2:
	movq	%cr2, %rax
	ret
rdcr3:
	movq	%cr3, %rax
	ret
rdcr4:
	movq	%cr4, %rax
	ret
rdcr8:
	movq	%cr8, %rax
	ret

invtlb:
	movq	%cr3, %rax
	movq	%rax, %cr3
	ret

pre_mtrr_change:
	cli
	movq	%cr4, %rax
	pushq	%rax

	movq	%cr0, %rcx
	bts	$30, %rcx
	btr	$29, %rcx
	movq	%rcx, %cr0
	wbinvd

	movq	%cr4, %rcx
	btr	$7, %rcx
	movq	%rcx, %cr4

	movl	$0x2ff, %edi
	call	rdmsr
	btr	$11, %rax
	movl	$0x2ff, %edi
	movq	%rax, %rsi
	call	wrmsr

	popq	%rax
	ret

post_mtrr_change:
	pushq	%rdi
	wbinvd
	movq	%cr3, %rax
	movq	%rax, %cr3

	movl	$0x2ff, %edi
	call	rdmsr
	bts	$11, %rax
	movl	$0x2ff, %edi
	movq	%rax, %rsi
	call	wrmsr

	movq	%cr0, %rcx
	btr	$30, %rcx
	btr	$29, %rcx
	movq	%rcx, %cr0

	popq	%rax
	movq	%rax, %cr4
	sti
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
