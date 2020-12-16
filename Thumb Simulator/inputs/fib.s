	.arch armv6
	.fpu softvfp
	.code	16
	.file	"fib.c"
	.text
	.align	2
	.global	main
	.code	16
	.thumb_func
	.type	main, %function
main:
	push	{r7, lr}	// 1
	sub	sp, sp, #16		// 1
	add	r7, sp, #0		// 1
	mov	r3, #0			// 1
	str	r3, [r7, #12]	// 1
	mov	r3, #1			// 1
	str	r3, [r7, #8]	// 1
	mov	r3, #0			// 1
	str	r3, [r7, #4]	// 1
	mov	r3, #0			// 1
	str	r3, [r7]		// 1
	b	.L2				// 4
.L3:
	ldr	r2, [r7, #12]	// 1
	ldr	r3, [r7, #8]	// 
	add	r3, r2, r3		// 4
	str	r3, [r7, #4]	// 1
	ldr	r3, [r7, #8]	//
	str	r3, [r7, #12]	// 4
	ldr	r3, [r7, #4]	//
	str	r3, [r7, #8]	// 4
	ldr	r3, [r7]		//
	add	r3, r3, #1		// 4
	str	r3, [r7]		// 1
.L2:
	ldr	r3, [r7]		// 
	cmp	r3, #9			// 4
	ble	.L3				// 4, 1, 6
	ldr	r3, [r7, #4]	// 
	mov	r0, r3			// 4
	mov	sp, r7			// 1
	add	sp, sp, #16		// 1
	@ sp needed
	pop	{r7, pc}		// 1
	.size	main, .-main
