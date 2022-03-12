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

; Convenções *************************
; Tipos de labels:
; - FUNC: rotinas sem efeitos colaterais a não ser a alteração
;	dos valores dos registradores de saída
; - MEMROT: rotinas sem efeitos colaterais a não ser a alteração
;	dos valores dos registradores de saída e de endereços
;	de memória esperados
; - PRIV: partes de rotinas que não deveriam ser chamadas diretamente por outras rotinas
; - Outros: rotinas sem garantias sobre efeitos colaterais. Pode mudar registradores
;
; Registradores:
; - Argumentos (entradas) são dados nos registradores R4, R5, ..., R9.
; - Retornos (saídas) são dados nos registradores R10, R11, R12
; - Variáveis temporárias são armazenadas nos registrados R13, R14, R15
;
; Essas convenções podem ser quebradas em caso de necessidade
; ************************************

MAIN:
		mov.w #EXPR, R4
		call #FUNC_COMPUTE_EXPRESSION

		jmp $
		nop

FUNC_COMPUTE_EXPRESSION:
		; Calcula uma expressão matemática com digitos, +, - e =
		; Entradas
		; R4 - Endereço da expressão a ser calculada (W)
		; Saídas
		; R5 - Resultado da expressão (W)
		; Temporários
		; R13 - Valor ASCII sendo analisado
		; R14 - Valor ASCII da próxima operação a ser realizada
		; R15 - Número por extenso sendo "montado"
		push.w R4
		push.w R10
		push.w R11
		push.w R13
		push.w R14
		push.w R15

		; Inicialização
		mov.w #0, R5
		mov.w #0x2B, R14 ; op +
		mov.w #0, R15

PRIV_FUNC_COMPUTE_EXPRESSION_LOOP:
		mov.b @R4+, R13

		push.w R4
		mov.b R13, R4
		call #FUNC_TRY_GET_DIGIT_FROM_ASCII
		pop.w R4

		tst.w R11
		jz PRIV_FUNC_COMPUTE_EXPRESSION_DIGIT_FOUND

		; Assumir que, se o caractere não é dígito, então é a próxima operação

		; Aplicar a operação atual, para depois salvar a nova operação
		cmp.b #0x2B, R14 ; op +
		jeq PRIV_FUNC_COMPUTE_EXPRESSION_APPLY_OP_PLUS

		cmp.b #0x2D, R14 ; op -
		jeq PRIV_FUNC_COMPUTE_EXPRESSION_APPLY_OP_SUB

PRIV_FUNC_COMPUTE_EXPRESSION_AFTER_OP:
		; Se a operação atual for =, finalizar
		cmp.b #0x3D, R10 ; op =
		jeq PRIV_FUNC_COMPUTE_EXPRESSION_END

		; Salvar a nova operação
		mov.w R10, R14
		mov.w #0, R15
		jmp PRIV_FUNC_COMPUTE_EXPRESSION_LOOP

PRIV_FUNC_COMPUTE_EXPRESSION_APPLY_OP_PLUS:
		add.w R15, R5
		jmp PRIV_FUNC_COMPUTE_EXPRESSION_AFTER_OP

PRIV_FUNC_COMPUTE_EXPRESSION_APPLY_OP_SUB:
		sub.w R15, R5
		jmp PRIV_FUNC_COMPUTE_EXPRESSION_AFTER_OP

PRIV_FUNC_COMPUTE_EXPRESSION_DIGIT_FOUND:
		push.w R4
		push.w R5
		mov.w R15, R4
		mov.w R10, R5
		call #FUNC_ADD_DIGIT
		pop.w R5
		pop.w R4

		mov.w R10, R15
		jmp PRIV_FUNC_COMPUTE_EXPRESSION_LOOP


PRIV_FUNC_COMPUTE_EXPRESSION_END:
		pop.w R15
		pop.w R14
		pop.w R13
		pop.w R11
		pop.w R10
		pop.w R4
		ret

FUNC_ADD_DIGIT:
		; Adiciona um dígito (não ASCII) à direita de um número em um registrador
		; Entradas
		; R4 - Número a ser acrescido (W)
		; R5 - Dígito a adicionar (W)
		; Saídas
		; R10 - Novo número (W)
		mov.w R4, R10

		mov.w R10, &MPY
		mov.w #10, &OP2
		mov.w &RES0, R10
		; R10 = N * 10

		add.w R5, R10
		; R10 = N * 10 + D

		ret

FUNC_TRY_GET_DIGIT_FROM_ASCII:
		; Obtem o valor de um dígito a partir do valor de seu caractere ASCII, se for um dígito
		; Entradas
		; R4 - Caractere ASCII (B)
		; Saídas
		; R10 - Valor do dígito, se for um dígito (W)
		; R11 - Flag indicadora de não-dígito (W)
		cmp.b #0x30, R4
		jl PRIV_FUNC_TRY_GET_DIGIT_FROM_ASCII_NON_DIGIT

		cmp.b #0x3A, R4
		jhs PRIV_FUNC_TRY_GET_DIGIT_FROM_ASCII_NON_DIGIT

PRIV_FUNC_TRY_GET_DIGIT_FROM_ASCII_IS_DIGIT:
		mov.w #0, R10
		add.b R4, R10
		sub.w #0x30, R10
		mov.w #0, R11
		ret

PRIV_FUNC_TRY_GET_DIGIT_FROM_ASCII_NON_DIGIT:
		mov.w R4, R10
		mov.w #1, R11
		ret


; ------------------------------------------------------------------------------
; Data
; ------------------------------------------------------------------------------
		.data
EXPR:	.byte	"729+1041+213+47-30-500-253+7-4=" ; = 1250
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
            
