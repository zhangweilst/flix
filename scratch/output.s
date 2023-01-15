# x64 assembly hex to ascii utilities
# sysv x64 calling convention

	.file	"output.s"
	.text
	.code64

	.globl	hex_to_ascii
	.globl	byte_to_ascii
	.globl	word_to_ascii
	.globl	long_to_ascii
	.globl	quad_to_ascii

hex_to_ascii:
	cmpq	$0xa, %rdi
	jge	1f
	addq	$48, %rdi
	jmp	2f
1:
	addq	$87, %rdi
2:
	movq	%rdi, %rax
	ret

byte_to_ascii:
	pushq	%rdi
	andq	$0xf, %rdi
	call	hex_to_ascii
	popq	%rdi
	movb	%al, %ch

	shr	$4, %rdi
	pushq	%rcx
	call	hex_to_ascii
	popq	%rcx
	movb	%al, %cl

	movw	%cx, %ax
	ret

word_to_ascii:
	pushq	%rdi
	andq	$0xff, %rdi
	call	byte_to_ascii
	popq	%rdi
	movw	%ax, %cx
	shl	$16, %ecx

	shr	$8, %rdi
	pushq	%rcx
	call	byte_to_ascii
	popq	%rcx
	movw	%ax, %cx

	movl	%ecx, %eax
	ret

long_to_ascii:
	pushq	%rdi
	andq	$0xffff, %rdi
	call	word_to_ascii
	popq	%rdi
	movl	%eax, %ecx
	shl	$32, %rcx

	shr	$16, %rdi
	pushq	%rcx
	call	word_to_ascii
	popq	%rcx

	orq	%rcx, %rax
	ret
	
# output in %rdx:%rax
quad_to_ascii:
	pushq	%rdi
	movq	$0xffffffff, %rax
	andq	%rax, %rdi
	call	long_to_ascii
	popq	%rdi
	movq	%rax, %rdx

	shr	$32, %rdi
	pushq	%rdx
	call	long_to_ascii
	popq	%rdx
	
	ret
