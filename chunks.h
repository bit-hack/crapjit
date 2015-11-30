#pragma once
#include <stdint.h>

enum jit_op {
    ins_const,
    ins_getl,
    ins_setl,
    ins_add,
    ins_sub,
    ins_mul,
    ins_div,
    ins_lt,
    ins_lte,
    ins_gt,
    ins_gte,
    ins_eq,
    ins_neq,
    ins_and,
    ins_or,
    ins_notl,
    ins_dup,
    ins_drop,
    ins_sink,
    ins_frame,
    ins_ret,
    ins_call,
    ins_jnz,
    ins_jz,
    ins_jmp
};

struct jit_chunk_t {
    jit_op op_;
    uint32_t size_;
    int32_t abs_op_;
    int32_t rel_op_;
    const char * data_;
};
