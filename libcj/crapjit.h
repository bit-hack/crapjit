#pragma once
#include <cstdint>
#include <vector>

#include "runasm.h"

namespace cj {

struct ir_t {

  enum type_t {
    IR_CONST,
    IR_DROP,
    IR_DUP,
    IR_SINK,
    IR_GETL,
    IR_SETL,
    IR_FRAME,
    IR_RETURN,
    IR_CALL,
    IR_JZ,
    IR_JNZ,
    IR_JMP,
    IR_LABEL,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_AND,
    IR_OR,
    IR_NOTL,
    IR_LT,
    IR_LEQ,
    IR_GT,
    IR_GEQ,
    IR_EQ,
    IR_NEQ,
  } type;

  ir_t(type_t type, uint32_t imm)
    : type(type)
    , imm_(imm)
  {
  }

  ir_t(type_t type, ir_t &target)
    : type(type)
    , target_(&target)
  {
  }

  ir_t(type_t type)
    : type(type)
    , imm_(0)
  {
  }

  void target(ir_t& t) {
    target_ = &t;
  }

  union {
    uint32_t imm_;
    ir_t* target_;
  };
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
    , inst_(nullptr)
  {
  }

  reloc_t(uint8_t* base, type_t type, const ir_t *ir)
    : base_(base)
    , type_(type)
    , inst_(ir)
  {
  }

  bool isValid() const {
    return base_ != nullptr;
  }

  void set(label_t);

  type_t      type_;  // rellcation type
  uint8_t*    base_;  // code pointer relocation is needed
  const ir_t* inst_;  // instruction reloc applies to
};

struct crapjit_t {

    crapjit_t();
    ~crapjit_t();

    void    emit_const  (int32_t);
    void    emit_drop   (uint32_t);
    void    emit_dup    ();
    void    emit_sink   (uint32_t);

    void    emit_getl   (int32_t);
    void    emit_setl   (int32_t);

    void    emit_frame  (uint32_t);
    void    emit_return (uint32_t);

    ir_t&   emit_call   ();

    ir_t&   emit_jz     ();
    ir_t&   emit_jnz    ();
    ir_t&   emit_jmp    ();

    ir_t&   emit_label  ();

    void    emit_add    ();
    void    emit_sub    ();
    void    emit_mul    ();

    void    emit_and    ();
    void    emit_or     ();
    void    emit_notl   ();

    void    emit_lt     ();
    void    emit_leq    ();
    void    emit_gt     ();
    void    emit_geq    ();
    void    emit_eq     ();
    void    emit_neq    ();

    void* finish();

    void clear();

    std::vector<ir_t*> ir;

protected:
    ir_t& push_    (const ir_t& inst);
    void  dumpCode_(const uint8_t *until);

    uint32_t codegen_     (runasm::runasm_t& x86, uint32_t index, reloc_t& reloc);
    uint32_t codegenCmp_  (runasm::runasm_t& x86, uint32_t index, reloc_t& reloc);
    uint32_t codegenConst_(runasm::runasm_t& x86, uint32_t index, reloc_t& reloc);

    uint8_t *code_;
    size_t code_size_;
};

}  // namespace cj
