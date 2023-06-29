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

; To do
;	-function to take pointer to sprite and write it to a video address

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
	; params:		3 params in registers - param order right to left, with register order: AX, DX, BX, CX
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
	; calling conv: WATCOM_C (registers filled, followed by stack)
	; description:	draws a pixel at x,y pixel position with specified 2-byte color
	; params:		(3 params in registers, with register order: AX, DX, BX, CX
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		x position			= ax
	;				[in]		y position			= dx
	;				[in]		color				= bx
	;				[return]	none

    %stacksize	large							; tell NASM to use bp

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


	;call	vga_get_pixel_mem_addr				; get address within selected 64K VRAM segment, returned in bx
	;inline below
		and		dx,			0b00000000_00011111
		shl		dx,			11							; the lower 5 y bits correspond with the line number within VRAM segment
		shl		ax,			1							; each x position = 2 bytes, so double bx
		or		ax,			dx							; yyyyyxxx_xxxxxxxx = address within VRAM 64K segment


	mov		si,				0xa000
	mov		es,				si
	
	;mov		ax,				[color]				; fill pixel with color
	mov		si,				ax
	mov		word es:[si],	bx					; fill pixel with color
	
	pop		es
	pop		si
	pop		dx
	pop		bx
	pop		ax
	
	ret

global _vga_draw_rect_filled
_vga_draw_rect_filled:
	; calling conv: C (stack)
	; to do: change over to WATCOMC calling convention for performance improvement
	; description:	draws a filled rectangle - *currently start vals must be lower than end vals
	; params:		5 params on stack
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		start x				= bp+4
	;				[in]		start y				= bp+6
	;				[in]		end x				= bp+8
	;				[in]		end y				= bp+10
	;				[in]		color				= bp+12
	; TO DO bounds checks

	%stacksize	large							; tell NASM to use bp
    %arg		startx:word, starty:word, endx:word, endy:word, color:word

	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer

	push	ax
	push	dx
	push	bx

	
	inc		word [endx]				; param - number is inclusive of rect - add 1 to support loop code strucure below
	inc		word [endy]				; param - number is inclusive of rect - add 1 to support loop code strucure below

	mov		bx,	[color]
	mov		dx,	[starty]
	mov		ax,	[startx]
	.row:
		call	vga_draw_pixel_faster_					; ax=x,dx=y,bx=color
		inc		ax										; move right a pixel
		cmp		ax,					[endx]				; compare current x to end of row of rectangle
		jne		.row
	mov		ax,					[startx]					; start over on x position
	inc		dx											; move down a pixel
	cmp		dx,					[endy]					; compare current y to end of column of rectangle
	jne		.row

	pop		bx
	pop		dx
	pop		ax
	
	mov		sp,		bp
	pop		bp
	ret		; for c++ calls, don't need to pop -- caller must pop params off stack!

global vga_draw_circle_filled_
vga_draw_circle_filled_:
	; calling conv: WATCOMC (registers filled, followed by stack)
	; param order right to left, with register order: AX, DX, BX, CX
	; description:	draws a filled circle at position x,y with given radius and color
	; oldparams:	[in]	dx = x coordinate of circle center
	;				[in]	di = y coordinate of circle center
	;				[in]	bx = radius
	;				[in]	ax = color
	; params:		[in]	ax = x coordinate of circle center
	;				[in]	dx = y coordinate of circle center
	;				[in]	bx = radius
	;				[in]	cx = color
	;				[out]	none
	; notes:		adapted from http://computer-programming-forum.com/45-asm/67a67818aff8a94a.htm
	; lots of clean-up and optimization needed here when time permits

	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	es
	push	bp
	
	;/// temporary shuffling to support updated params using WATCOMC calling conv -- need to rework procedure to get rid of this
	push	ax
	push	dx
	push	cx
	pop		ax
	pop		di
	pop		dx
	;///

	mov		bp,		0			; x coordinate
    mov     si,		bx			; y coordinate

	.c00:        
		call	.8pixels            ; Set 8 pixels
		sub     bx,			bp		; D=D-X
		inc     bp                  ; X+1
		sub     bx,			bp      ; D=D-(2x+1)
		jg      .c01                ; >> no step for Y
		add     bx,			si      ; D=D+Y
		dec     si                  ; Y-1
		add     bx,			si      ; D=D+(2Y-1)
	.c01:        
		cmp     si,			bp      ; Check X>Y
		jae     .c00                ; >> Need more pixels
		jmp		.out
	.8pixels:   
		call      .4pixels          ; 4 pixels
	.4pixels:   
		xchg      bp,		si      ; Swap x and y //   bp as x to bp as y  -and- si as y to si as x
		call      .2pixels          ; 2 pixels
	.2pixels:   
		neg		si
		push    di
		push	bp
		add		di,			si			; rectangle start & end y
		add		bp,			dx

		mov		cx,			bp			; rectangle end x
		pop		bp
		push	dx
		sub     dx,			bp			; rectangle start x

		push	ax						; pixel color
		push	di						; rectangle end y (also = start y)
		push	cx						; rectangle end x
		push	di						; rectangle start y (alse = end y)
		push	dx						; rectangle start x
		call	_vga_draw_rect_filled	; params on stack, reverse order
		add		sp,		10				; as caller, need to pop 5 params off stack
		
		pop		dx
		pop     di
		ret
	
	.out:
		pop		bp
		pop		es
		pop		si
		pop		di
		pop		dx
		pop		cx
		pop		bx
		pop		ax

		ret

global vga_draw_circle_
vga_draw_circle_:
	; calling conv: WATCOMC (registers filled, followed by stack)
	; description:	draws an unfilled circle at position x,y with given radius and color
	; params:		[in]	dx = x coordinate of circle center
	;				[in]	di = y coordinate of circle center
	;				[in]	bx = radius
	;				[in]	ax = color
	;				[out]	none
	; params:		[in]	ax = x coordinate of circle center
	;				[in]	dx = y coordinate of circle center
	;				[in]	bx = radius
	;				[in]	cx = color
	;				[out]	none
	; notes:		adapted from http://computer-programming-forum.com/45-asm/67a67818aff8a94a.htm
	; lots of clean-up and optimization needed here when time permits

	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	es
	push	bp

	;/// temporary shuffling to support updated params using WATCOMC calling conv -- need to rework procedure to get rid of this
	push	ax
	push	dx
	push	cx
	pop		ax
	pop		di
	pop		dx
	;///

	mov		bp,		0			; x coordinate
    mov     si,		bx			; y coordinate

	.c00:        
		call	.8pixels            ; Set 8 pixels
		sub     bx,			bp		; D=D-X
		inc     bp                  ; X+1
		sub     bx,			bp      ; D=D-(2x+1)
		jg      .c01                ; >> no step for Y
		add     bx,			si      ; D=D+Y
		dec     si                  ; Y-1
		add     bx,			si      ; D=D+(2Y-1)
	.c01:        
		cmp     si,			bp      ; Check X>Y
		jae     .c00                ; >> Need more pixels
		jmp		.out
	.8pixels:   
		call      .4pixels          ; 4 pixels
	.4pixels:   
		xchg      bp,		si      ; Swap x and y //   bp as x to bp as y  -and- si as y to si as x
		call      .2pixels          ; 2 pixels
	.2pixels:   
		neg		si
		push    di
		push	bp
		add		di,			si
		add		bp,			dx

		push	ax					; pixel color
		push	di					; pixel y
		push	bp					; pixel x
		call	_vga_draw_pixel		; pixels for right side of circle
		add		sp,			6		; pop 6 bytes from stack

		pop		bp
		push	dx
		sub     dx,			bp

		push	ax					; pixel color
		push	di					; pixel y
		push	dx					; pixel x
		call	_vga_draw_pixel		; pixels for left side of circle
		add		sp,			6		; pop 6 bytes from stack
		
		pop		dx
		pop     di
		ret
	
	.out:
		pop		bp
		pop		es
		pop		si
		pop		di
		pop		dx
		pop		cx
		pop		bx
		pop		ax
		ret

global _vga_draw_rect
_vga_draw_rect:
	; calling conv: C (stack)
	; to do: change over to WATCOMC calling convention for performance improvement
	; description:	draws a  rectangle - *currently start vals must be lower than end vals
	; params:		(5 params on stack - push params on stack in reverse order of listing below)
	;				*old base pointer				= bp
	;				*return address					= bp+2
	;				[in]		start_x 			= bp+4
	;				[in]		start_y 			= bp+6
	;				[in]		end_x				= bp+8
	;				[in]		end_y 				= bp+10
	;				[in]		color x				= bp+12
	;				[return]	none
	;
	; TO DO bounds checks

	%stacksize	large            ; tell NASM to use bp
    %arg		start_x:word, start_y:word, end_x:word, end_y:word, color:word
	
	push	bp									; save base pointer
	mov		bp,				sp					; update base pointer to current stack pointer

	push	ax
	push	bx
	push	cx
	push	dx

	mov		cx,	[end_x]
	inc		word [end_x]	;[vga_rect_end_x]		; param - number is inclusive of rect - add 1 to support loop code strucure below
	mov		dx, [end_y]
	inc		word [end_y] ;[vga_rect_end_y]		; param - number is inclusive of rect - add 1 to support loop code strucure below

	mov		bx,					[start_y]
	mov		ax,					[start_x]
	.row:
		; if x = first or last column, or y = first or last row, then draw pixel
		cmp		ax,	[start_x]
		je		.draw_pixel
		cmp		ax, cx
		je		.draw_pixel
		cmp		bx, [start_y]
		je		.draw_pixel
		cmp		bx, dx
		je		.draw_pixel

		jmp		.skip_pixel			; not first or last colomn or row, so skip this pixel

		.draw_pixel:
		push	word [color]				; push color to stack (param for vga_draw_pixel)
		push	bx							; push y position to stack (param for vga_draw_pixel)
		push	ax							; push x position to stack (param for vga_draw_pixel)
		call	_vga_draw_pixel
		
		.skip_pixel:
		inc		ax										; move right a pixel
		cmp		ax,					[end_x]				; compare current x to end of row of rectangle
		jne		.row
	mov		ax,					[start_x]					; start over on x position
	inc		bx											; move down a pixel
	cmp		bx,					[end_y]					; compare current y to end of column of rectangle
	jne		.row

	pop		dx
	pop		cx
	pop		bx
	pop		ax
	
	mov		sp,		bp
	pop		bp		;or 'leave'
	
	ret		; for c++ calls, don't need to pop -- caller must pop params off stack!

