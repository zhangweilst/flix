# call wrappers for uefi calling convention
# just don't want to link gnu-efi libs

	.file	"call_wrappers.s"
	.text
	.code64

	.globl	efi_call0
	.globl	efi_call1
	.globl	efi_call2
	.globl	efi_call3
	.globl	efi_call4
	.globl	efi_call5
	.globl	efi_call6
	.globl	efi_call7
	.globl	efi_call8
	.globl	efi_call9
	.globl	efi_call10

efi_call0:
	subq	$40, %rsp
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call1:
	subq	$40, %rsp
	movq	%rsi, %rcx
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call2:
	subq	$40, %rsp
	movq	%rsi, %rcx
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call3:
	subq	$40, %rsp
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call4:
	subq	$40, %rsp
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call5:
	subq	$40, %rsp
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$40, %rsp
	ret

efi_call6:
	subq	$56, %rsp
	movq	56+8(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$56, %rsp
	ret

efi_call7:
	subq	$56, %rsp
	movq	56+16(%rsp), %rax
	movq	%rax, 48(%rsp)
	movq	56+8(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$56, %rsp
	ret

efi_call8:
	subq	$72, %rsp
	movq	72+24(%rsp), %rax
	movq	%rax, 56(%rsp)
	movq	72+16(%rsp), %rax
	movq	%rax, 48(%rsp)
	movq	72+8(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$72, %rsp
	ret

efi_call9:
	subq	$72, %rsp
	movq	72+32(%rsp), %rax
	movq	%rax, 64(%rsp)
	movq	72+24(%rsp), %rax
	movq	%rax, 56(%rsp)
	movq	72+16(%rsp), %rax
	movq	%rax, 48(%rsp)
	movq	72+8(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$72, %rsp
	ret

efi_call10:
	subq	$88, %rsp
	movq	88+40(%rsp), %rax
	movq	%rax, 72(%rsp)
	movq	88+32(%rsp), %rax
	movq	%rax, 64(%rsp)
	movq	88+24(%rsp), %rax
	movq	%rax, 56(%rsp)
	movq	88+16(%rsp), %rax
	movq	%rax, 48(%rsp)
	movq	88+8(%rsp), %rax
	movq	%rax, 40(%rsp)
	movq	%r9, 32(%rsp)
	movq	%r8, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	call	*%rdi
	addq	$88, %rsp
	ret
