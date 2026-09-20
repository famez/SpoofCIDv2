.syntax unified
.cpu cortex-m0
.thumb

.section .isr_vector,"a",%progbits
.word _estack
.word Reset_Handler
.rept 46
.word 0
.endr

.text
.thumb_func
.global Reset_Handler
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    bl main
    b .
