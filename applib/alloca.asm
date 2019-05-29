[BITS 32]

        GLOBAL      __alloca

[SECTION .text]

__alloca:
    SUB     ESP, EAX
    ADD     EAX, 4
    JMP     DWORD [ESP + EAX - 4]       ; RETの代わり