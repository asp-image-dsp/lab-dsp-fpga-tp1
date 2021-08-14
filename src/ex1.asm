; -----------------------------------------------------------------
;	file					ex1.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 1
; -----------------------------------------------------------------			

; -----------------------------------------------------------------
; Program Memory
; -----------------------------------------------------------------
			ORG			P:$E000
main	EQU			*
			; Initialization of accumulators and registers
			MOVE		#$FFFFFF,A0
			MOVE		#$FFFFFF,A1
			MOVE 		#$FF,A2
			MOVE		#$FFFFFF,B0
			MOVE		#$FFFFFF,B1
			MOVE 		#$FF,B2
			MOVE		#$FFFFFF,X0
			MOVE		#$FFFFFF,X1
			; Main code
			MOVE		#$3D,X1
			MOVE		#$3D,A1
			MOVE		#$3D,B
			END			main