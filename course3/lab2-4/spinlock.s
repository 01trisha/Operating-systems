	.arch armv8-a
	.file	"spinlock.c"
	.text
	.align	2
	.global	spinlock_init
	.type	spinlock_init, %function
spinlock_init:
.LFB0:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	str	wzr, [sp, 20]
	ldr	w0, [sp, 20]
	mov	w1, w0
	ldr	x0, [sp, 24]
	stlr	w1, [x0]
	nop
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE0:
	.size	spinlock_init, .-spinlock_init
	.global	__aarch64_cas4_acq_rel
	.align	2
	.global	spinlock_lock
	.type	spinlock_lock, %function
spinlock_lock:
.LFB1:
	.cfi_startproc
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	mov	x29, sp
	stp	x19, x20, [sp, 16]
	.cfi_offset 19, -48
	.cfi_offset 20, -40
	str	x0, [sp, 40]
.L6:
	str	wzr, [sp, 52]
	ldr	x0, [sp, 40]
	str	x0, [sp, 56]
	mov	w0, 1
	str	w0, [sp, 48]
	ldr	w0, [sp, 48]
	mov	w1, w0
	ldr	x0, [sp, 56]
	add	x19, sp, 52
	ldr	w20, [x19]
	mov	x2, x0
	mov	w0, w20
	bl	__aarch64_cas4_acq_rel
	cmp	w0, w20
	mov	w1, w0
	cset	w0, eq
	cmp	w0, 0
	bne	.L3
	str	w1, [x19]
.L3:
	and	w0, w0, 1
	cmp	w0, 0
	bne	.L8
	bl	sched_yield
	b	.L6
.L8:
	nop
	nop
	ldp	x19, x20, [sp, 16]
	ldp	x29, x30, [sp], 64
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE1:
	.size	spinlock_lock, .-spinlock_lock
	.align	2
	.global	spinlock_unlock
	.type	spinlock_unlock, %function
spinlock_unlock:
.LFB2:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 8]
	ldr	x0, [sp, 8]
	str	x0, [sp, 24]
	str	wzr, [sp, 20]
	ldr	w0, [sp, 20]
	mov	w1, w0
	ldr	x0, [sp, 24]
	stlr	w1, [x0]
	nop
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE2:
	.size	spinlock_unlock, .-spinlock_unlock
	.ident	"GCC: (Debian 13.1.0-6) 13.1.0"
	.section	.note.GNU-stack,"",@progbits
