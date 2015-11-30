#pragma once
#include <stdint.h>
#include "chunks.h"

typedef uint8_t* label_t;

struct reloc_t {

    reloc_t()
        : type_(INVALID)
        , base_(nullptr)
    {
    }

    void set(label_t);
    void imm_i32(int32_t);
    void imm_u32(uint32_t);

    enum {
        INVALID,
        RELOC_ABS,
        RELOC_REL,
    }
    type_;

    uint8_t * base_;
};

struct crapjit_t {

    crapjit_t();
    ~crapjit_t();

    void    emit_const  (int32_t);
    void    emit_drop   ();
    void    emit_dup    ();
    void    emit_sink   (uint32_t);

    void    emit_getl   (int32_t);
    void    emit_setl   (int32_t);

    void    emit_frame  (uint32_t);
    void    emit_return (uint32_t);

    reloc_t emit_call   ();
    
    reloc_t emit_jz     ();
    reloc_t emit_jnz    ();
    reloc_t emit_jmp    ();

    label_t emit_label  ();

    void    emit_add    ();
    void    emit_sub    ();
    void    emit_mul    ();
    void    emit_div    ();

    void    emit_and    ();
    void    emit_or     ();
    void    emit_not    ();

    void    emit_lt     ();
    void    emit_leq    ();
    void    emit_gt     ();
    void    emit_geq    ();
    void    emit_eq     ();
    void    emit_neq    ();

protected:
    reloc_t emit_inst(uint8_t * & ptr, jit_op op);

    uint8_t * data_;
    uint8_t * head_;
    bool      did_label_;
};
