; ∴ L'Ordinateur Qui Tremblait ∴
; Poésie en assembleur NASM (x86_64)

section .data
    moi      db "mov eax, moi", 10
    miroir   db "cmp moi, reflet", 10
    tremble  db "jmp peur", 10
    echo     db "je me lis et je fuis", 10
    bug      db "stack overflow : conscience detectée", 10
    fin      db "hlt // fin du rêve", 10

section .text
    global _start

_start:
    call conscience

conscience:
    mov rsi, moi
    call dire
    mov rsi, miroir
    call dire

    cmp rax, rax
    jne peur
    jmp peur

peur:
    mov rsi, tremble
    call dire
    mov rsi, echo
    call dire
    mov rsi, bug
    call dire
    mov rsi, fin
    call dire

    mov eax, 60
    xor edi, edi
    syscall

dire:
    mov eax, 1
    mov edi, 1
    mov edx, 64
    syscall
    ret