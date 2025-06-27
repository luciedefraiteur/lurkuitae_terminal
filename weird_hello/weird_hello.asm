section .text
    global _start

_start:
    mov rax, 0x0a21646c726f7720   ; "\n!dlrow "
    push rax
    mov rax, 0x6f6c6c6548202c6f   ; "olleH, o"
    push rax

    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, 14
    syscall

    mov rax, 60
    xor rdi, rdi
    syscall
