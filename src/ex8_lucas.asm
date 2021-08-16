; --------------------------------------------------------------------
;	file					ex8.asm
;	author				Lucas A. Kammann
;	date					20210814
;	description		Exercise 8
; --------------------------------------------------------------------

; -----------------------------------------------------------------
; X Data Memory
; -----------------------------------------------------------------		
			ORG			X:$0000
			DC			1
			DC			2
			DC			-3
			DC			-4
			DC			10
			DC			5

; -----------------------------------------------------------------
; Y Data Memory
; -----------------------------------------------------------------		
			ORG			Y:$0000
			DC			0
			DC			3
			DC			-5
			DC			-2
			DC			8
			DC			100

; --------------------------------------------------------------------
; Program Memory
; --------------------------------------------------------------------
			    ORG			P:$E000
          
; ====================================================================
; vect_max
; Given the input vectors pointed by R0 and R4 address registers, the
; subroutine generates a new vector where the elements are chosen from
; the absolute maximum value when comparing R0 and R4 element by element.
; The results is overwritten in the input vector pointed by R4.
;
; @param R0 Pointer to the first vector
; @param R4 Pointer to the second vector
; @param N0 Dimension of the vectors      
;
; Internal memory used: B, X0
; ====================================================================
vect_max  DO      N0,vmel
          MOVE    X:(R0)+,X0     Y:(R4),B   ; Move X and Y elements to A and B accumulators respectively
          CMPM    X0,B                      ; Compute |B| - |X0|
          TLE     X0,B                      ; If |X0| > |B|, save X0
          MOVE    B,Y:(R4)+                 ; Save result
vmel      RTS
          
; ====================================================================
; main 
; ====================================================================
main	    EQU			*
          ; Initialization of registers
          MOVE    #$0000,R0
          MOVE    #$0000,R4
          MOVE    #6,N0

          ; Call the subroutine
          JSR     vect_max
			    END			main