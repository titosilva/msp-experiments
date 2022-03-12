;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------


VISTO1:		mov.w #MSG_CLARA, R5
			mov.w #MSG_CIFR, R6
			call #ENIGMA

			mov.w #MSG_CIFR, R5
			mov.w #DCF, R6
			call #ENIGMA

			jmp $
			nop

ENIGMA:
			; Inputs
			; R5 - Address of the message to process
			; R6 - Address where to write the output
			; Outputs
			; R15 - Temporary variable
			; R14 - Holder of RT_STATES
			; R13 - Holder of RT_REFS
			push R4
			push R5
			push R14
			push R15

			call #ROUT_SET_ROTOR_AND_REFLECTOR_REFS
			mov.w #RT_REFS, R13

			mov.w #RT_STATES, R14
			mov.w #CHAVE, R15

			mov.b 2(R15), 0(R14)
			mov.b 6(R15), 1(R14)
			mov.b 10(R15), 2(R14)

PRIV_ENIGMA_LOOP:
			mov.b @R5, R15
			tst.b R15
			jz PRIV_ENIGMA_END

			mov.b R15, R4
			call #FUNC_IN_ACCEPTABLE_RANGE
			tst.b R10
			jz PRIV_ENIGMA_WRITE

			push R5
			push R6

			call #FUNC_GET_NUMBER_FROM_ASCII

			mov.b R10, R4
			mov.w 0(R13), R5
			mov.b 0(R14), R6
			call #FUNC_APPLY_ROTOR_WITH_CONF_TO_NUMBER

			mov.b R10, R4
			mov.w 2(R13), R5
			mov.b 1(R14), R6
			call #FUNC_APPLY_ROTOR_WITH_CONF_TO_NUMBER

			mov.b R10, R4
			mov.w 4(R13), R5
			mov.b 2(R14), R6
			call #FUNC_APPLY_ROTOR_WITH_CONF_TO_NUMBER

			mov.b R10, R4
			mov.w 6(R13), R5
			call #FUNC_APPLY_ROTOR_TO_NUMBER

			mov.b R10, R4
			mov.w 4(R13), R5
			mov.b 2(R14), R6
			call #FUNC_APPLY_REVERSED_ROTOR_WITH_CONF_TO_NUMBER

			mov.b R10, R4
			mov.w 2(R13), R5
			mov.b 1(R14), R6
			call #FUNC_APPLY_REVERSED_ROTOR_WITH_CONF_TO_NUMBER

			mov.b R10, R4
			mov.w 0(R13), R5
			mov.b 0(R14), R6
			call #FUNC_APPLY_REVERSED_ROTOR_WITH_CONF_TO_NUMBER

			; Rotate the rotors to the next configurations
			mov.w R14, R4
			mov.w #RT_ROT_COUNT, R5
			mov.b #3, R6
			call #ROUT_ROTATE_ROTORS
			; --------------------------------------------

			pop R6
			pop R5

			mov.b R10, R4
			call #FUNC_GET_ASCII_FROM_NUMBER
			mov.b R10, R4

PRIV_ENIGMA_WRITE:
			mov.b R4, 0(R6)
			inc.w R5
			inc.w R6
			jmp PRIV_ENIGMA_LOOP

PRIV_ENIGMA_END:
			pop R15
			pop R14
			pop R5
			pop R4
			ret

; ROTOR FUNCTIONS --------------------------------------------------
FUNC_APPLY_ROTOR_TO_NUMBER:
			; Inputs
			; R4 - Number to apply the rotor on (B)
			; R5 - Rotor to be used (W)
			; Outputs
			; R10 - Result (B)
			push R5

			add.w R4, R5
			mov.b @R5, R10

			pop R5
			ret

FUNC_APPLY_ROTOR_WITH_CONF_TO_NUMBER:
			; Inputs
			; R4 - Number to apply the rotor on (B)
			; R5 - Rotor to be used (W)
			; R6 - Configuration to be used (B)
			; Outputs
			; R10 - Result (B)
			push R4
			push R6
			push R15

			push R5

			add.b R6, R4
			mov.w #RT_TAM, R15
			mov.b @R15, R5
			call #FUNC_GET_REMAINDER_UB

			mov.b R10, R4
			pop R5
			call #FUNC_APPLY_ROTOR_TO_NUMBER

			pop R15
			pop R6
			pop R4

			ret


FUNC_APPLY_REVERSED_ROTOR_TO_NUMBER:
			; Inputs
			; R4 - Number to apply the rotor on (B)
			; R5 - Rotor to be used (W)
			; Outputs
			; R10 - Result (B)
			push R5

			mov.b #0, R10

PRIV_APPLY_REVERSED_ROTOR_TO_NUMBER_LOOP:
			cmp.b @R5, R4
			jeq PRIV_APPLY_REVERSED_ROTOR_TO_NUMBER_END

			inc.w R5
			inc.b R10
			jmp PRIV_APPLY_REVERSED_ROTOR_TO_NUMBER_LOOP

PRIV_APPLY_REVERSED_ROTOR_TO_NUMBER_END:
			pop R5
			ret

FUNC_APPLY_REVERSED_ROTOR_WITH_CONF_TO_NUMBER:
			; Inputs
			; R4 - Number to apply the rotor on (B)
			; R5 - Rotor to be used (W)
			; R6 - Configuration to be used (B)
			; Outputs
			; R10 - Result (B)
			push R4
			push R5
			push R15

			call #FUNC_APPLY_REVERSED_ROTOR_TO_NUMBER

			mov.w #RT_TAM, R15
			add.b @R15, R10
			sub.b R6, R10

			mov.b R10, R4
			mov.b @R15, R5
			call #FUNC_GET_REMAINDER_UB

			pop R15
			pop R5
			pop R4

			ret

ROUT_ROTATE_ROTORS:
			; Inputs
			; R4 - Vector of rotor configurations (W)
			; R5 - Vector of rotor rotations counting (W)
			; R6 - Quantity of rotors in the vector (B)
			push R4
			push R5
			push R6
			push R15

			jmp PRIV_ROUT_ROTATE_ROTORS_INC

PRIV_ROUT_ROTATE_ROTORS_LOOP:
			dec.b R6
			jn PRIV_ROUT_ROTATE_ROTORS_END

			mov.w #RT_TAM, R15
			cmp.b @R15, -1(R5)
			jne PRIV_ROUT_ROTATE_ROTORS_END

			mov.b #0, -1(R5)

PRIV_ROUT_ROTATE_ROTORS_INC:
			dec.b 0(R4)

			jn PRIV_ROUT_ROTATE_ROTORS_INC_NEG_REACHED

PRIV_ROUT_ROTATE_ROTORS_INC_2:
			inc.b 0(R5)
			inc.w R4
			inc.w R5

			jmp PRIV_ROUT_ROTATE_ROTORS_LOOP

PRIV_ROUT_ROTATE_ROTORS_INC_NEG_REACHED:
			mov.w #RT_TAM, R15
			mov.b @R15, 0(R4)
			dec.b 0(R4)
			jmp PRIV_ROUT_ROTATE_ROTORS_INC_2


PRIV_ROUT_ROTATE_ROTORS_END:
			pop R15
			pop R6
			pop R5
			pop R4
			ret

; CHAR FUNCTIONS ---------------------------------------------------
FUNC_GET_NUMBER_FROM_ASCII:
			; Inputs
			; R4 - ASCII character to process
			; Outpus
			; R10 - Number corresponding to the ASCII character
			push R4
			sub.b #0x40, R4
			mov.b R4, R10
			pop R4
			ret

FUNC_GET_ASCII_FROM_NUMBER:
			; Inputs
			; R4 - Number to process
			; Outpus
			; R10 - ASCII character value
			push R4
			add.b #0x40, R4
			mov.b R4, R10
			pop R4
			ret

FUNC_IN_ACCEPTABLE_RANGE:
			; Inputs
			; R4 - ASCII value to look at (B)
			; Outputs
			; R10 - Byte indicating whether the value is inside the acceptable range (B)
			cmp.b #0x40, R4
			jl FUNC_IN_ACCEPTABLE_RANGE_FALSE
			cmp.b #0x60, R4
			jhs FUNC_IN_ACCEPTABLE_RANGE_FALSE

			mov.b #1, R10
			ret

FUNC_IN_ACCEPTABLE_RANGE_FALSE:
			mov.b #0, R10
			ret


; MODULAR ARITHMETICS ----------------------------------------------
FUNC_GET_REMAINDER_UB:
			; Inputs
			; R4 - Dividend (B)
			; R5 - Divisor (B)
			; Outputs
			; R10 - Remainder (B)
			push R4

PRIV_GET_REMAINDER_UB_LOOP:
			cmp.b R5, R4
			jl PRIV_GET_REMAINDER_UB_END
			sub.b R5, R4
			jmp PRIV_GET_REMAINDER_UB_LOOP

PRIV_GET_REMAINDER_UB_END:
			mov.b R4, R10

			pop R4
			ret

; ROTOR MANAGEMENT --------------------------------------------------
ROUT_SET_ROTOR_AND_REFLECTOR_REFS:
			; Sets the RT_REFS vector according to CHAVE
			; R14, R15 - Temporary variable
			push R5
			push R14
			push R15

			mov.w #RT1, R5
			mov.w #RT_REFS, R14
			mov.w #CHAVE, R15

			mov.w 0(R15), R4
			call #FUNC_GET_ROTOR_REF_FROM_NUMBER
			mov.w R10, 0(R14)

			mov.w 4(R15), R4
			call #FUNC_GET_ROTOR_REF_FROM_NUMBER
			mov.w R10, 2(R14)

			mov.w 8(R15), R4
			call #FUNC_GET_ROTOR_REF_FROM_NUMBER
			mov.w R10, 4(R14)

			mov.w #RF1, R5

			mov.w 12(R15), R4
			call #FUNC_GET_ROTOR_REF_FROM_NUMBER
			mov.w R10, 6(R14)

			pop R15
			pop R14
			pop R5
			ret

FUNC_GET_ROTOR_REF_FROM_NUMBER:
			; Inputs
			; R4 - Number of the rotor (B)
			; R5 - Base rotor (W)
			; Outputs
			; R10 - Reference
			push R4
			push R15

			dec.b R4 ; We start counting from 1 on CHAVE
			mov.w R5, R10

FUNC_GET_ROTOR_REF_FROM_NUMBER_LOOP:
			dec.b R4
			jn FUNC_GET_ROTOR_REF_FROM_NUMBER_END
			mov.w #RT_TAM, R15
			add.w 0(R15), R10
			jmp FUNC_GET_ROTOR_REF_FROM_NUMBER_LOOP


FUNC_GET_ROTOR_REF_FROM_NUMBER_END:
			pop R15
			pop R4
			ret

;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
; Área de dados
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			.data
; Chave = A, B, C, D, E, F, G
;A = número do rotor à esquerda e B = sua configuração;
;C = número do rotor central e D = sua configuração;
;E = número do rotor à direita e F = sua configuração;
;G = número do refletor.
; A,B C,D E,F G
CHAVE: .word 3,5, 4,8, 1,3, 3 ;<<<=========== Chave Obrigatória

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Área com os dados do Enigma. Não os altere ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
RT_TAM: .word 32 ;Tamanho
RT_QTD: .word 05 ;Quantidade de Rotores
RF_QTD: .word 03 ;Quantidade de Refletores

VAZIO: .space 12 ;Para facilitar o endereço dos rotores

;Rotores com 32 posições
ROTORES:
RT1: 	.byte 2, 24, 14, 23, 31, 17, 16, 8, 15, 21, 25, 12, 9, 29, 6, 0
		.byte 19, 30, 7, 4, 11, 26, 10, 18, 13, 5, 20, 22, 1, 27, 28, 3
RT2: 	.byte 29, 0, 21, 2, 11, 13, 5, 16, 17, 10, 23, 6, 12, 26, 3, 7
 		.byte 14, 8, 20, 19, 24, 1, 30, 9, 22, 4, 31, 18, 28, 25, 27, 15
RT3: 	.byte 0, 31, 1, 12, 17, 28, 26, 19, 27, 22, 3, 9, 15, 16, 7, 21
		.byte 11, 24, 6, 29, 10, 25, 5, 30, 23, 4, 8, 14, 20, 13, 18, 2
RT4: 	.byte 12, 2, 7, 25, 21, 22, 5, 0, 3, 30, 28, 6, 8, 18, 11, 14
		.byte 23, 16, 26, 19, 31, 4, 15, 20, 10, 27, 1, 29, 17, 24, 9, 13
RT5: 	.byte 3, 8, 0, 13, 16, 7, 12, 21, 5, 15, 6, 9, 24, 30, 4, 22
		.byte 23, 28, 25, 27, 14, 19, 10, 1, 2, 26, 11, 17, 29, 20, 31, 18

;Refletores com 32 posições
REFLETORES:
RF1: 	.byte 23, 4, 13, 10, 1, 25, 19, 18, 11, 24, 3, 8, 14, 2, 12, 30
		.byte 27, 26, 7, 6, 29, 31, 28, 0, 9, 5, 17, 16, 22, 20, 15, 21

RF2: 	.byte 5, 7, 20, 6, 18, 0, 3, 1, 12, 23, 17, 29, 8, 30, 25, 26
		.byte 24, 10, 4, 31, 2, 27, 28, 9, 16, 14, 15, 21, 22, 11, 13, 19

RF3:	.byte 19, 15, 11, 17, 10, 30, 31, 26, 20, 23, 4, 2, 16, 25, 22, 1
		.byte 12, 3, 29, 0, 8, 27, 14, 9, 28, 13, 7, 21, 24, 18, 5, 6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MSG_CLARA:		.byte       "UMA NOITE DESTAS, VINDO DA CIDADE PARA O ENGENHO NOVO, ENCONTREI NO TREM DA CENTRAL UM RAPAZ AQUI DO BAIRRO, QUE EU CONHECO DE VISTA E DE CHAPEU.@MACHADO\ASSIS",0     ;Mensagem em claro
MSG_CIFR:		.byte       "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",0     ;Mensagem cifrada - BFCD BFAD AFBF AFE
DCF:			.byte       "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",0     ;Mensagem decifrada

; Configurações dos rotores
RT_STATES:		.byte	0, 0, 0
RT_ROT_COUNT:	.byte	0, 0, 0
RT_REFS:		.word	0, 0, 0, 0

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
