; -----------------------------------------------------------------
;	file					ex5.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 5
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
			MOVEC		#$0700,SR
			; Code here
			MOVE		#$400000,X0
			ADD			X0,A
			ADD			X0,A
			END			main