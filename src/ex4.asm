; -----------------------------------------------------------------
;	file					ex4.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 4
; -----------------------------------------------------------------			

; -----------------------------------------------------------------
; Program Memory
; -----------------------------------------------------------------
			ORG			P:$E000
main	EQU			*
			; Initialization of accumulators and registers
			MOVE		#$800000,A0
			MOVE		#$000123,A1
			MOVE		#$00,A2
			MOVE		#$FFFFFF,B0
			MOVE		#$000000,B1
			MOVE		#$FF,B2
			MOVE		#$400000,X0
			MOVE		#$400000,X1
			; Code here
			MACR		X0,X1,A
			RND			B
			MPYR		X0,X1,B
			END			main