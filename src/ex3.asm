; -----------------------------------------------------------------
;	file					ex3.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 3
; -----------------------------------------------------------------			

; -----------------------------------------------------------------
; Program Memory
; -----------------------------------------------------------------
			ORG			P:$E000
main	EQU			*
			; Initialization of accumulators and registers
			MOVE		#$000000,A0
			MOVE		#$A00000,A1
			MOVE		#$00,A2
			MOVE		#$000000,Y0
			MOVE		#$000000,Y1
			MOVEC		#$0000,SR
			; Code here
			MOVE		A1,X1
			MOVE		A,Y1
			MOVE		A,R7
			MOVE		A1,X0
			END			main