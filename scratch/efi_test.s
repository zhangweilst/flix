#
# calling convention note:
# for efi related calls, use windows x64 calling convention
# for others, use sysv x64 calling convention
#

	.data

hello:
	.ascii	"hello world from zhangweilst\n\r"
hello_utf16:
	.word	72, 10, 13, 0
abc:
	.ascii	"ABC"
xyz:
	.ascii	"XYZ"

ascii:
	.fill	1024, 1, 0
utf16:
	.fill	1024, 1, 0
null_table:
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
utf16_new_line:
	.byte	0x0a, 0x00, 0x0d, 0x00, 0x00, 0x00

efi_image_handle:
	.quad	0
efi_system_table:
	.quad	0
efi_con_out_handle:
	.quad	0
efi_con_out:
	.quad	0
efi_con_output_string:
	.quad	0
efi_con_clear_screen:
	.quad	0


	.file	"startup.s"
	.text
	.globl	start, _start
	.globl	jump_to_kernel
	.code64

start:
_start:
	movq	%rcx, %rdi
	movq	%rdx, %rsi
	andq	$~0xf, %rsp
	call	efi_main

	jmp	relax

jump_to_kernel:
	jmp	*%rdi

# initialize functions and variables
# input:
#	%rdi: image_handle
# 	%rsi: efi_system_table
# output:
#	none
initialize:
	movq	%rdi, efi_image_handle(%rip)
	movq	%rsi, efi_system_table(%rip)
	movq	56(%rsi), %rax
	movq	%rax, efi_con_out_handle(%rip)
	movq	64(%rsi), %rax
	movq	%rax, efi_con_out(%rip)
	movq	8(%rax), %rcx
	movq	%rcx, efi_con_output_string(%rip)
	movq	48(%rax), %rcx
	movq	%rcx, efi_con_clear_screen(%rip)
	ret

# reset the processor
# input:
# 	none
# output:
#	none, never return
reboot:
	lidt	null_table(%rip)
	int3

# relax the processor
# input:
# 	none
# output:
#	none, never return
relax:
	nop
	jmp relax

# convert an ascii string to utf-16 string
# input:
#	%rdi: src ascii string
#	%rsi: src length
# output:
#	%rax: utf-16 string address
ascii_to_utf16:
	movq	%rsi, %rcx
	movq	%rdi, %rsi
	leaq	utf16(%rip), %rdi
1:
	movsb
	movb	$0, (%rdi)
	incq	%rdi
	decq	%rcx
	jnz	1b

	movw	$0, (%rdi)
	leaq	utf16(%rip), %rax
	ret

# say hello to people before the screen
# input:
# 	none
# output:
#	none
say_hello:
	leaq	hello(%rip), %rdi
	movq	$30, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

peek_cr4:
	call	println
	movq	%cr4, %rdi
	call	printq
	ret

peek_ia32_efer:
	call	println
	movq	$0xC0000080, %rcx
	rdmsr
	movq	%rax, %rdi
	shlq	$32, %rdx
	orq	%rdx, %rdi
	call	printq
	ret

peed_addr_size:
	movq	$0x80000008, %rax
	cpuid
	xorq	%rcx, %rcx
	movw	%ax, %cx
	movq	%rcx, %rdi
	call	printw  # MAXLINEARADDR:MAXPHYADDR
	ret

peek_gdt:
	# gdtr contents
	call	println
	sgdt	null_table(%rip)
	leaq	null_table(%rip), %rcx
	movw	0(%rcx), %ax
	movq	%rax, %rdi
	call	printw
	leaq	null_table(%rip), %rcx
	movq	2(%rcx), %rdi
	call	printq

	# gdt contents
	call	println
	leaq	null_table(%rip), %rax
	pushq	%rbx
	pushq	%rbp
	xorq	%rbx, %rbx
	movw	0(%rax), %bx
	movq	2(%rax), %rbp
	addq	%rbp, %rbx
1:
	cmpq	%rbx, %rbp
	jae	2f
	movq	(%rbp), %rdi
	call	printq
	addq	$8, %rbp
	jmp	1b
2:
	popq	%rbp
	popq	%rbx
	ret

peek_page_table:
	movq	%cr3, %rdi
	movq	%rdi, null_table(%rip)
	call	printq
	call	println

	movq	null_table(%rip), %rax
	movq	(%rax), %rax
	andq	$~0xfff, %rax
	movq	(%rax), %rdi
	call	printq
	
clear_screen:
	movq	efi_con_out(%rip), %rcx
	call	*efi_con_clear_screen(%rip)
	ret

# print 4-bit hex digit in ascii
# input:
# 	%rdi: hex digint in lower 4-bit of %rdi
# output:
#	print in the screen
printh:
	call	hex_to_ascii
	leaq	ascii(%rip), %rcx
	movb	%al, 0(%rcx)
	movb	$10, 1(%rcx)
	movb	$13, 2(%rcx)
	movq	%rcx, %rdi
	movq	$3, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

# print 1-byte hex digit in ascii
# input:
# 	%rdi: hex digit in lower byte of %rdi
# output:
#	print in the screen
printb:
	call	byte_to_ascii
	leaq	ascii(%rip), %rcx
	movw	%ax, 0(%rcx)
	movb	$10, 2(%rcx)
	movb	$13, 3(%rcx)
	movq	%rcx, %rdi
	movq	$4, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

# print 2-bytes hex digit in ascii
# input:
# 	%rdi: hex digit in lower 2 bytes of %rdi
# output:
#	print in the screen
printw:
	call	word_to_ascii
	leaq	ascii(%rip), %rcx
	movl	%eax, 0(%rcx)
	movb	$10, 4(%rcx)
	movb	$13, 5(%rcx)
	movq	%rcx, %rdi
	movq	$6, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

# print 4-bytes hex digit in ascii
# input:
# 	%rdi: hex digit in lower 4 bytes of %rdi
# output:
#	print in the screen
printl:
	call	long_to_ascii
	leaq	ascii(%rip), %rcx
	movq	%rax, 0(%rcx)
	movb	$10, 8(%rcx)
	movb	$13, 9(%rcx)
	movq	%rcx, %rdi
	movq	$10, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

# print 8-bytes hex digit in ascii
# input:
# 	%rdi: hex digit in %rdi
# output:
#	print in the screen
printq:
	call	quad_to_ascii
	leaq	ascii(%rip), %rcx
	movq	%rax, 0(%rcx)
	movq	%rdx, 8(%rcx)
	movb	$10, 16(%rcx)
	movb	$13, 17(%rcx)
	movq	%rcx, %rdi
	movq	$18, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	ret

# print an empty line
# input:
# 	none
# output:
#	print a new line in the screen
println:
	movq	efi_con_out(%rip), %rcx
	leaq	utf16_new_line(%rip), %rdx
	call	*efi_con_output_string(%rip)
	ret

task_abc:
	leaq	abc(%rip), %rdi
	movq	$3, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	jmp	task_xyz

task_xyz:
	leaq	xyz(%rip), %rdi
	movq	$3, %rsi
	call	ascii_to_utf16
	movq	efi_con_out(%rip), %rcx
	movq	%rax, %rdx
	call	*efi_con_output_string(%rip)
	jmp	task_abc

