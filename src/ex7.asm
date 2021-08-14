; -----------------------------------------------------------------
;	file					ex7.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 7
; -----------------------------------------------------------------		

; -----------------------------------------------------------------
; X Data Memory
; -----------------------------------------------------------------		
			ORG			X:$0000
			DC			$10FEDC
			DC			$210FED
			DC			$4210FE
			DC			$84210F
			DC			$D84210
			DC			$FB8421
		
; -----------------------------------------------------------------
; Program Memory
; -----------------------------------------------------------------
			ORG			P:$E000
main	EQU			*
			; Initialization of accumulators and registers
			MOVE		#$0000,R0
			MOVE		#$0000,R4
			MOVEC		#$0800,SR
			MOVE		#$FFFF,M0
			MOVE		#$FFFF,M4
			; Main code
			MOVE		X:(R0)+,A
			REP			#6
			MOVE		A,Y:(R4)+	X:(R0)+,A
			JLC			OK
			BSET		#0,Y:$100
OK		BCLR		#6,SR
			END			main