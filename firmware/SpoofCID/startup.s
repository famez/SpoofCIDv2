.syntax unified
.cpu cortex-m0
.thumb

.section .isr_vector,"a",%progbits
.word _estack
.word Reset_Handler
/* IRQ 0..5: no usadas */
.rept 6
.word 0
.endr
/* IRQ 6 = system call / no usada */
.word 0
/* IRQ 7 = udb_interrupt: UDB StatusReg levanta esta IRQ cuando cmd2/cmd9 activos */
.word UDB_IRQHandler
/* IRQ 8..46: no usadas */
.rept 39
.word 0
.endr

.extern UDB_IRQHandler

.text
.thumb_func
.global Reset_Handler
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    bl main
    b .
