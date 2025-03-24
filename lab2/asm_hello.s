    .global _start

    .section .data
msg:    .asciz "Hello, world!\n"
len = . - msg

    .section .text
_start:
    //system call write
    mov x0, 1
    ldr x1, =msg
    mov x2, len
    mov x8, 15 //number of system call of write
    svc #0

    mov x0, 0 
    mov x8, 93
    svc #0

