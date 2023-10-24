#pragma once
#include <cstdint>
#include <list>

#include "chunks.h"

namespace cj {

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

  uint8_t* base_;
};

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
    IR_DIV,
    IR_AND,
    IR_OR,
    IR_NOT,
    IR_LT,
    IR_LEQ,
    IR_GT,
    IR_GEQ,
    IR_EQ,
    IR_NEQ,
  } type;

  ir_t(type_t type, uint32_t imm)
    : type(type)
    , _imm(imm)
  {
  }

  ir_t(type_t type, ir_t &target)
    : type(type)
    , _target(&target)
  {
  }

  ir_t(type_t type)
    : type(type)
    , _imm(0)
  {
  }

  void target(ir_t& t) {
    _target = &t;
  }

  union {
    uint32_t _imm;
    ir_t*    _target;
  };
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

    ir_t&   emit_call   ();
    
    ir_t&   emit_jz     ();
    ir_t&   emit_jnz    ();
    ir_t&   emit_jmp    ();

    ir_t&   emit_label  ();

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

    void*   finish      ();

protected:
    ir_t& _push(const ir_t& inst);
    reloc_t _emit_inst(uint8_t*& ptr, jit_op op);

    std::list<ir_t> ir;

    uint8_t *data_;
    uint8_t *head_;
};

}  // namespace cj
