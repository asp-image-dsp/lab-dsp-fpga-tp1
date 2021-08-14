; -----------------------------------------------------------------
;	file					ex2.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 2
; -----------------------------------------------------------------		

; -----------------------------------------------------------------
; Program Memory
; -----------------------------------------------------------------
			ORG			P:$E000
main	EQU			*
			; Initialization of accumulators and registers
			MOVE		#$000000,A0
			MOVE		#$000000,A1
			MOVE		#$00,A2
			MOVE		#$000000,B0
			MOVE		#$000000,B1
			MOVE		#$00,B2
			MOVE		#$000000,X0
			MOVE		#$000000,X1
			; Main code
			MOVE		#$CABA00,X1
			MOVE		X1,A
			MOVE		X1,B1
			END			main