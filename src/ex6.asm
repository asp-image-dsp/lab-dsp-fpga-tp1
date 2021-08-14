; -----------------------------------------------------------------
;	file					ex6.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 6
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
			MOVE		#$600000,X0
			MOVE		#$0C0000,X1
			MOVE		#$0000,R0
			; Main code
			ADD			X1,A
			REP			#$A
			NORM		R0,A
			ADD			X0,A
			END			main