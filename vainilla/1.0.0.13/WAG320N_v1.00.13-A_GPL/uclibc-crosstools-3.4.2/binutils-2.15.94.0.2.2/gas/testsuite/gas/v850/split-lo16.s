	movhi	hi(foo),r0,r1
	addi	lo(foo),r1,r2
	ld.b	lo(foo),r1,r2
	ld.bu	lo(foo),r1,r2

	ld.bu	lo(0x12345),r1,r2
	ld.bu	lo(0x123456),r1,r2
