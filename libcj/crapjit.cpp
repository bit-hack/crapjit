#include <cstring>
#include <map>

#include "crapjit.h"
#include "chunks.h"
#include "system.h"


#if defined(_MSC_VER)
#define assert(X) { if (!(X)) __debugbreak(); }
#else
#include <cassert>
#endif

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
  case ir_t::IR_NOT:    return ins_notl;
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

extern jit_chunk_t chunk_table[];

template <typename type_t>
static void insert(uint8_t * & ptr, const type_t val) {
    const uint32_t size = sizeof(type_t);
    memcpy(ptr, &val, size);
    ptr += size;
}

static void insert(uint8_t * & ptr, const void * data, const uint32_t size) {
    memcpy(ptr, data, size);
    ptr += size;
}

crapjit_t::crapjit_t() {
    data_ = head_ = (uint8_t*)code_alloc(1024 * 512);
}

crapjit_t::~crapjit_t() {
    code_free(data_);
}

void reloc_t::set(label_t pos) {
    assert(base_[2] == 0xbb);
    switch (type_) {
    case RELOC_ABS:
      insert<label_t>(base_, pos);
      break;
    case RELOC_REL: {
      const int32_t org = int32_t(base_) + 4;
      const int32_t dst = int32_t(pos);
      insert<int32_t>(base_, dst - org);
      break;
    }
    default:
      assert(!"unreachable");
    }
}

void reloc_t::imm_i32(int32_t val) {
    assert(type_ == RELOC_ABS);
    assert(base_[2] == 0xbb);
    insert(base_, val);
}

void reloc_t::imm_u32(uint32_t val) {
    assert(type_ == RELOC_ABS);
    assert(base_[2] == 0xbb);
    insert(base_, val);
}

reloc_t crapjit_t::_emit_inst(uint8_t * & ptr, jit_op op) {

    const jit_chunk_t & chunk = chunk_table[op];

    reloc_t reloc;

    uint8_t *base = ptr;

    insert(ptr, chunk.data_, chunk.size_);

    // has absolute offset
    if (chunk.abs_op_ >= 0) {
        reloc.type_ = reloc.RELOC_ABS;
        reloc.base_ = base += chunk.abs_op_;
    }

    // has relative offset
    if (chunk.rel_op_ >= 0) {
        reloc.type_ = reloc.RELOC_REL;
        reloc.base_ = base += chunk.rel_op_;
    }

    return reloc;
}

void crapjit_t::emit_const(int32_t val) {
    _push(ir_t{ ir_t::IR_CONST, uint32_t(val) });
}

void crapjit_t::emit_drop() {
    _push(ir_t{ ir_t::IR_DROP });
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

void crapjit_t::emit_frame(uint32_t val) {
    _push(ir_t{ ir_t::IR_FRAME, val });
}

void crapjit_t::emit_return(uint32_t val) {
    _push(ir_t{ ir_t::IR_RETURN, val });
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

  head_ = data_;

  // emit instruction stream
  for (const auto& i : ir) {

    if (i.type == ir_t::IR_LABEL) {
      labels[&i] = head_;
      continue;
    }

    jit_op op = inst_type(i.type);
    reloc_t reloc = _emit_inst(head_, op);

    switch (i.type) {
    case ir_t::IR_CONST:
    case ir_t::IR_GETL:
    case ir_t::IR_SETL:
      reloc.imm_i32(i._imm);
      break;
    case ir_t::IR_SINK:
    case ir_t::IR_FRAME:
    case ir_t::IR_RETURN:
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

  return data_;
}

}  // namespace cj
