#pragma once
#include <stdint.h>

namespace cj {

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

typedef uint8_t* label_t;

struct reloc_t {

  enum type_t {
    INVALID,
    RELOC_ABS,
    RELOC_REL,
  };

  reloc_t()
    : type_(INVALID)
    , base_(nullptr)
  {
  }

  reloc_t(uint8_t *base, type_t type)
    : base_(base)
    , type_(type)
  {
  }

  void set(label_t);
  void imm_i32(int32_t);
  void imm_u32(uint32_t);

  type_t   type_;
  uint8_t* base_;
};

reloc_t chunk_emit(uint8_t*& ptr, jit_op op);

}  // namespace cj
