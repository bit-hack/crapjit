[map all asm.map]
BITS 32
ORG 0x0
SECTION .text

; ---- push constant
ins_const:
    mov  eax, 0xaabbccdd
    push eax

; ---- get local
ins_getl:
    mov  eax, [ebp+0xaabbccdd]
    push eax

; ---- set local
ins_setl:
    pop eax
    mov [ebp+0xaabbccdd], eax

; ---- add
ins_add:
    pop eax;
    add [esp], eax;

; ---- sub
ins_sub:
    pop eax;
    sub [esp], eax;

; ---- multiply
ins_mul:
    pop eax;
    mul dword [esp];
    mov [esp], eax;

; ---- divide
ins_div:
    pop  eax;
    div dword [esp];
    mov  [esp], eax;

; ---- compare less than
ins_lt:
    pop  eax;
    pop  edx;
    cmp  edx, eax;
    setl al;
    and  eax, 1;
    push eax;

; ---- compare less than or equal
ins_lte:
    pop   eax;
    pop   edx;
    cmp   edx, eax;
    setle al;
    and   eax, 1;
    push  eax;

; ---- compare greater than
ins_gt:
    pop  eax;
    pop  edx;
    cmp  edx, eax;
    setg al;
    and  eax, 1;
    push eax;

; ---- compare greater than or equal
ins_gte:
    pop   eax;
    pop   edx;
    cmp   edx, eax;
    setge al;
    and   eax, 1;
    push  eax;

; ---- compare equal
ins_eq:
    pop  eax;
    pop  edx;
    cmp  edx, eax;
    sete al;
    and  eax, 1;
    push eax;

; ---- compare not equal
ins_neq:
    pop   eax;
    pop   edx;
    cmp   edx, eax;
    setne al;
    and   eax, 1;
    push  eax;

; ---- logical and
ins_and:
    pop  eax;
    pop  edx;
    and  eax, edx;
    push eax;

; ---- logical or
ins_or:
    pop  eax;
    pop  edx;
    or   eax, edx;
    push eax;

; ---- logical not
ins_notl:
    pop  eax;
    xor  eax, 1;
    push eax;

; ---- duplicate top item
ins_dup:
    mov  eax, [esp]
    push eax;

; ---- discard stack items
ins_drop:
    add esp, 0xaabbccdd;

; ---- pop items from under the topmost
ins_sink:
    pop  eax
    add  esp, 0xaabbccdd;
    push eax;
    
; ---- setup stack frame
ins_frame:
    push ebp;                       save base pointer
    mov  ebp, esp;                  new base pointer
    sub  esp, 0xaabbccdd;

; ---- return from subroutine
ins_ret:
    pop eax;                        return value
    pop ebp;                        restore base pointer
    add esp, 0xaabbccdd;
    ret;

; ---- call subroutine
ins_call:
    call 0xaabbccdd;
    push eax;                       save return value

; ---- jump if not zero
ins_jnz:
    pop eax;
    cmp eax, 0;
    jnz 0xaabbccdd;

; ---- jmp if zero
ins_jz:
    pop eax;
    cmp eax, 0;
    jz  0xaabbccdd;

; ---- jmp unconditional
ins_jmp:
    jmp 0xaabbccdd;
