	.dp
	.export R0
	.export R1
	.export R2
	.export R3
	.export R4
	.export R5
	.export R6
	.export R7

R0:	.word	0
	.word	0
R1:	.word	0
	.word	0
R2:	.word	0
	.word	0
R3:	.word	0
	.word	0
R4:	.word	0
	.word	0
R5:	.word	0
	.word	0
R6:	.word	0
	.word	0
R7:	.word	0
	.word	0

		.code
start:
		.word 0x80A8
		.byte 0x04			; 6809
		.byte 0x00			; 6309 not needed
		.byte >__code			; page to load at
		.byte 0				; no hints
		.word __code_size		; text size info
		.word __data_size		; data size info
		.word __bss_size		; bss size info
		.byte 16			; entry relative to start
		.word __end			; to help the emulator :-)
		.byte 0				; ZP not used on 6809

		jmp start2

		.code

start2:
		ldd	#0
		std	@zero
		ldd	#1
		std	@one

		; Set up _environ
		ldd 4,s
		std _environ

		jsr ___stdio_init_vars
		jsr	_main

		; return and exit
		jsr _exit

		.data
		.export _environ
_environ:	.word 0
