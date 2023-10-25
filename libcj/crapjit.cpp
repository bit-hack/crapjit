#include <cstring>
#include <map>

#include "crapjit.h"
#include "chunks.h"
#include "system.h"


namespace cj {

static jit_op inst_type(ir_t::type_t type) {
  switch (type) {
  case ir_t::IR_CONST:  return ins_const;
  case ir_t::IR_DROP:   return ins_drop;
  case ir_t::IR_DUP:    return ins_dup;
  case ir_t::IR_SINK:   return ins_sink;
  case ir_t::IR_GETL:   return ins_getl;
  case ir_t::IR_SETL:   return ins_setl;
  case ir_t::IR_FRAME:  return ins_frame;
  case ir_t::IR_RETURN: return ins_ret;
  case ir_t::IR_CALL:   return ins_call;
  case ir_t::IR_JZ:     return ins_jz;
  case ir_t::IR_JNZ:    return ins_jnz;
  case ir_t::IR_JMP:    return ins_jmp;
  case ir_t::IR_ADD:    return ins_add;
  case ir_t::IR_SUB:    return ins_sub;
  case ir_t::IR_MUL:    return ins_mul;
  case ir_t::IR_DIV:    return ins_div;
  case ir_t::IR_AND:    return ins_and;
  case ir_t::IR_OR:     return ins_or;
  case ir_t::IR_NOTL:   return ins_notl;
  case ir_t::IR_LT:     return ins_lt;
  case ir_t::IR_LEQ:    return ins_lte;
  case ir_t::IR_GT:     return ins_gt;
  case ir_t::IR_GEQ:    return ins_gte;
  case ir_t::IR_EQ:     return ins_eq;
  case ir_t::IR_NEQ:    return ins_neq;
  default:
    assert(!"unreachable");
    return jit_op(0);
  }
}

crapjit_t::crapjit_t()
  : code_(nullptr)
{
    code_ = (uint8_t*)code_alloc(1024 * 512);
}

crapjit_t::~crapjit_t() {
    code_free(code_);
}

void crapjit_t::emit_const(int32_t val) {
    _push(ir_t{ ir_t::IR_CONST, uint32_t(val) });
}

void crapjit_t::emit_drop(uint32_t count) {
    _push(ir_t{ ir_t::IR_DROP, count*4 });
}

void crapjit_t::emit_dup() {
    _push(ir_t{ ir_t::IR_DUP });
}

void crapjit_t::emit_sink(uint32_t val) {
    _push(ir_t{ ir_t::IR_SINK, val*4 });
}

void crapjit_t::emit_getl(int32_t offs) {
    _push(ir_t{ ir_t::IR_GETL, uint32_t(offs*4) });
}

void crapjit_t::emit_setl(int32_t offs) {
    _push(ir_t{ ir_t::IR_SETL, uint32_t(offs*4) });
}

void crapjit_t::emit_frame(uint32_t offs) {
    _push(ir_t{ ir_t::IR_FRAME, uint32_t(offs*4) });
}

void crapjit_t::emit_return(uint32_t offs) {
    _push(ir_t{ ir_t::IR_RETURN, uint32_t(offs * 4) });
}

ir_t &crapjit_t::emit_call() {
    return _push(ir_t{ ir_t::IR_CALL });
}

ir_t& crapjit_t::emit_jz() {
    return _push(ir_t{ ir_t::IR_JZ });
}

ir_t& crapjit_t::emit_jnz() {
    return _push(ir_t{ ir_t::IR_JNZ });
}

ir_t& crapjit_t::emit_jmp() {
    return _push(ir_t{ ir_t::IR_JMP });
}

ir_t& crapjit_t::emit_label() {
    return _push(ir_t{ ir_t::IR_LABEL });
}

void crapjit_t::emit_add() {
    _push(ir_t{ ir_t::IR_ADD });
}

void crapjit_t::emit_sub() {
    _push(ir_t{ ir_t::IR_SUB });
}

void crapjit_t::emit_mul() {
    _push(ir_t{ ir_t::IR_MUL });
}

void crapjit_t::emit_div() {
    _push(ir_t{ ir_t::IR_DIV });
}

void crapjit_t::emit_and() {
    _push(ir_t{ ir_t::IR_AND });
}

void crapjit_t::emit_or() {
    _push(ir_t{ ir_t::IR_OR });
}

void crapjit_t::emit_notl() {
    _push(ir_t{ ir_t::IR_NOTL });
}

void crapjit_t::emit_lt() {
    _push(ir_t{ ir_t::IR_LT });
}

void crapjit_t::emit_leq() {
    _push(ir_t{ ir_t::IR_LEQ });
}

void crapjit_t::emit_gt() {
    _push(ir_t{ ir_t::IR_GT });
}

void crapjit_t::emit_geq() {
    _push(ir_t{ ir_t::IR_GEQ });
}

void crapjit_t::emit_eq() {
    _push(ir_t{ ir_t::IR_EQ });
}

void crapjit_t::emit_neq() {
    _push(ir_t{ ir_t::IR_NEQ });
}

ir_t& crapjit_t::_push(const ir_t& inst) {
    ir.push_back(inst);
    return ir.back();
}
void* crapjit_t::finish() {

  std::map<const ir_t*, label_t> labels;
  std::map<const ir_t*, reloc_t> relocs;

  runasm::runasm_t x86{ code_, 1024 };

  // emit instruction stream
  for (const auto& i : ir) {

    if (i.type == ir_t::IR_LABEL) {
      labels[&i] = x86.head();
      continue;
    }

    jit_op op = inst_type(i.type);
    reloc_t reloc = chunk_emit(x86, op);

    switch (i.type) {
    case ir_t::IR_CONST:
    case ir_t::IR_GETL:
    case ir_t::IR_SETL:
      reloc.imm_i32(i._imm);
      break;
    case ir_t::IR_SINK:
    case ir_t::IR_FRAME:
    case ir_t::IR_RETURN:
    case ir_t::IR_DROP:
      reloc.imm_u32(i._imm);
      break;
    case ir_t::IR_JZ:
    case ir_t::IR_JNZ:
    case ir_t::IR_JMP:
    case ir_t::IR_CALL:
      relocs[&i] = reloc;
      break;
    }
  }

  // apply relocations
  for (const auto &i : relocs) {
    const ir_t *inst   = i.first;
    const ir_t *target = inst->_target;
    label_t ptr = labels[target];
    reloc_t rel = i.second;
    rel.set(ptr);
  }

  return code_;
}

}  // namespace cj
