#
# calling convention note:
# for efi related calls, use windows x64 calling convention
# for others, use sysv x64 calling convention
#

	.data

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

# relax the processor
# input:
# 	none
# output:
#	none, never return
relax:
	pause
	jmp relax
