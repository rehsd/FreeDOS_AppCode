; Notes
; https://www.stefanobordoni.cloud/howto-easily-use-nasm-into-windows-c-c-applications/
; https://stackoverflow.com/questions/5858996/enter-and-leave-in-assembly
;Predefined __cdecl alias
;    #pragma aux __cdecl "_*"                            \
;                parm caller []                          \
;                value struct float struct routine [ax] \
;                modify [ax bx cx dx es]
;Note the following:
;	All symbols are preceded by an underscore character.
;	Arguments are pushed on the stack from right to left. That is, the last argument is pushed first. The calling routine will remove the arguments from the stack.
;	Floating-point values are returned in the same way as structures. When a structure is returned, the called routine allocates space for the return value, and returns a pointer to the return value in register AX.
;	Registers AX, BX, CX and DX, and segment register ES aren't saved and restored when a call is made.
;

%include "macros.asm"

cpu			286
bits 		16
sectalign	off

section .data align=2			;GLOBAL_ALIGNMENT
	%include "_data.asm"
section .bss align=2
	%include "_bss.asm"
section .text align=2

global _add2Nums
_add2Nums:
	; calling conv: C
	; description:	add two numbers
	; params:		(2 params on stack - push params on stack in reverse order of listing below)
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		num1 				= bp+4
	;				[in]		num2 				= bp+6
	;				[return]	sum of numbers		= ax

	;%push		mycontext						; save the current context 
	%stacksize	large							; tell NASM to use bp
    %arg		num1:word, num2:word
	
	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer

	; save any other modified registers
	; function code here
	mov		ax,		[num1]
	add		ax,		[num2]

	; restore any saved registers (other than ax which is used for return value)
	mov		sp,		bp
	pop		bp		;or 'leave'
	
	ret		;4					; return, pop 4 bytes for 2 params off stack
	;%pop

global vga_swap_frame_
vga_swap_frame_:
	; calling conv: WATCOM_C (registers filled, followed by stack)
	; description:	swaps frames on VGA card
	; params:		none
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[return]	none
	
	%stacksize	large							; tell NASM to use bp
	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer

	push	ax
	ds0000

	in		ax,			VGA_REG		;0x00a0	;VGA_REG
	xor		ax,			0b0000_0000_0001_0000	; bit4 = 286 active frame (VGA out active frame is opposite automatically)
	out		VGA_REG,	ax		;0x00a0, ax	;VGA_REG,	ax

	; TO DO implement interrupt support on VGA card as alternative to the following status polling
	.wait:
		in		ax,	VGA_REG		;0x00a0 ;VGA_REG
		test	ax, 0b1000_0000_0000_0000		; Looking for bit15 to be 0. Value 1 = frame switch is pending (tied to VSYNC).
		jnz		.wait							; wait
	
	ds0000out
	pop		ax

	mov		sp,		bp
	pop		bp		;or 'leave'

	ret

global _vga_draw_pixel
_vga_draw_pixel:
	; calling conv: C
	; description:	draws a pixel at x,y pixel position with specified 2-byte color
	; params:		(3 params on stack - push params on stack in reverse order of listing below)
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		x position			= bp+4
	;				[in]		y position			= bp+6
	;				[in]		color				= bp+8
	;				[return]	none
	
	%stacksize	large							; tell NASM to use bp
    %arg		x:word, y:word, color:word

	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer
	;ds0000
	push	ax
	push	bx
	push	dx
	push	si
	push	es

	mov		dx,				[y]					; pass y position in dx
	call	vga_set_segment_from_xy				; update VGA segment based on y position
	mov		bx,				[x]					; pass x position in bx (y already be in dx)
	call	vga_get_pixel_mem_addr				; get address within selected 64K VRAM segment, returned in bx

	mov		si,				0xa000
	mov		es,				si
	mov		ax,				[color]				; fill pixel with color
	mov		word es:[bx],	ax					; fill pixel with color
	
	pop		es
	pop		si
	pop		dx
	pop		bx
	pop		ax
	;ds0000out

	mov		sp,		bp
	pop		bp
	ret		;6									; return, pop 6 bytes for 3 params off stack

global vga_draw_pixel_fast_
vga_draw_pixel_fast_:
	; calling conv: WATCOM_C (registers filled, followed by stack)
	; description:	draws a pixel at x,y pixel position with specified 2-byte color
	; params:		(3 params in registers - param order right to left, with register order: AX, DX, BX, CX
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		x position			= bx
	;				[in]		y position			= dx
	;				[in]		color				= ax
	;				[return]	none

    %stacksize	large							; tell NASM to use bp

	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer
	
	;ds0000
	push	ax
	push	bx
	push	dx
	push	si
	push	es

	;mov		dx,				[y]					; pass y position in dx
	call	vga_set_segment_from_xy				; update VGA segment based on y position
	;mov		bx,				[x]					; pass x position in bx (y already be in dx)
	call	vga_get_pixel_mem_addr				; get address within selected 64K VRAM segment, returned in bx

	mov		si,				0xa000
	mov		es,				si
	
	;mov		ax,				[color]				; fill pixel with color
	mov		word es:[bx],	ax					; fill pixel with color
	
	pop		es
	pop		si
	pop		dx
	pop		bx
	pop		ax
	;ds0000out
	mov		sp,		bp
	pop		bp
	ret											; return, pop 0 bytes for 0 params off stack

vga_set_segment_from_xy:
	; description:	updates VGA register to set active segment based on y position
	; params:		[in]		dx		= y position
	;				[return]	none

	push	ax
	push	dx

	and		dx,			0b0000_0001_1110_0000		; these bits correspond to the VRAM segment (0-15)
	shr		dx,			5
	in		ax,			VGA_REG						; read the VGA register
	and		ax,			0b11111111_11110000			; read the register, keep all bits except segment
	or		ax,			dx							; update segment bits
	out		VGA_REG,	ax							; update VGA register
	
	pop		dx
	pop		ax
	ret

vga_get_pixel_mem_addr:
	; description:	gets the memory address of a pixel (within a 64K VRAM segment), given its x and y
	;				does not update the VGA register for active segment (use vga_set_segment_from_xy)
	; params:		[in]		dx		= pixel y position
	;				[in]		bx		= pixel x position
	;				[return]	bx		= memory address

	;push	ax
	push	dx
	
	and		dx,			0b00000000_00011111
	shl		dx,			11							; the lower 5 y bits correspond with the line number within VRAM segment
	shl		bx,			1							; each x position = 2 bytes, so double bx
	or		bx,			dx							; yyyyyxxx_xxxxxxxx = address within VRAM 64K segment
	
	pop		dx
	;pop		ax
	ret

global vga_draw_pixel_faster_
vga_draw_pixel_faster_:
	; description:	draws a pixel at x,y pixel position with specified 2-byte color
	; params:		(3 params in registers - param order right to left, with register order: AX, DX, BX, CX
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		x position			= bx
	;				[in]		y position			= dx
	;				[in]		color				= ax
	;				[return]	none

    %stacksize	large							; tell NASM to use bp

	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer
	
	push	ax
	push	bx
	push	dx
	push	si
	push	es

	;mov		dx,				[y]					; pass y position in dx
	;call	vga_set_segment_from_xy				; update VGA segment based on y position
	;inline below
		push	ax
		push	dx
		and		dx,			0b0000_0001_1110_0000		; these bits correspond to the VRAM segment (0-15)
		shr		dx,			5
		in		ax,			VGA_REG						; read the VGA register
		and		ax,			0b11111111_11110000			; read the register, keep all bits except segment
		or		ax,			dx							; update segment bits
		out		VGA_REG,	ax							; update VGA register
		pop		dx
		pop		ax


	;mov		bx,				[x]					; pass x position in bx (y already be in dx)
	;call	vga_get_pixel_mem_addr				; get address within selected 64K VRAM segment, returned in bx
	;inline below
		;push	ax
		;push	dx
		and		dx,			0b00000000_00011111
		shl		dx,			11							; the lower 5 y bits correspond with the line number within VRAM segment
		shl		bx,			1							; each x position = 2 bytes, so double bx
		or		bx,			dx							; yyyyyxxx_xxxxxxxx = address within VRAM 64K segment
		;pop		dx
		;pop		ax


	mov		si,				0xa000
	mov		es,				si
	
	;mov		ax,				[color]				; fill pixel with color
	mov		word es:[bx],	ax					; fill pixel with color
	
	pop		es
	pop		si
	pop		dx
	pop		bx
	pop		ax
	
	mov		sp,		bp
	pop		bp
	ret