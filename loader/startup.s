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
	.globl	cpu_relax
	.code64

start:
_start:
	movq	%rcx, %rdi
	movq	%rdx, %rsi
	andq	$~0xf, %rsp
	call	efi_main

	jmp	cpu_relax

jump_to_kernel:
	jmp	*%rdi

# relax the processor
# input:
# 	none
# output:
#	none, never return
cpu_relax:
	pause
	jmp cpu_relax
