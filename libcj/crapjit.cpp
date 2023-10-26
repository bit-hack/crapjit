#include <cstring>
#include <cassert>
#include <map>

#include "crapjit.h"
#include "runasm.h"

#if defined(_MSC_VER)
#include <Windows.h>

extern void* code_alloc(uint32_t size) {
  return VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

extern void code_free(void* ptr) {
  assert(ptr);
  VirtualFree(ptr, 0, MEM_RELEASE);
}
#else
#include <sys/mman.h>
#include "system.h"

extern void* code_alloc(uint32_t size) {
  const int prot = PROT_EXEC | PROT_READ | PROT_WRITE;
  const int flag = MAP_ANONYMOUS | MAP_PRIVATE;
  void* mem = mmap(nullptr, size, prot, flag, -1, 0);
  return mem;
}

extern void code_free(void* ptr) {
  munmap(ptr, 0);
}
#endif

template <typename type_t>
static void insert(uint8_t*& ptr, const type_t val) {
  const uint32_t size = sizeof(type_t);
  memcpy(ptr, &val, size);
  ptr += size;
}


namespace cj {

crapjit_t::crapjit_t()
  : code_(nullptr)
  , code_size_(1024 * 512)
{
    code_ = (uint8_t*)code_alloc(code_size_);
}

crapjit_t::~crapjit_t() {
    clear();
    code_free(code_);
}

void crapjit_t::clear() {
    for (ir_t* i : ir) {
        delete i;
    }
    ir.clear();
}

void crapjit_t::emit_const(int32_t val) {
    push_(ir_t{ ir_t::IR_CONST, uint32_t(val) });
}

void crapjit_t::emit_drop(uint32_t count) {
    push_(ir_t{ ir_t::IR_DROP, count*4 });
}

void crapjit_t::emit_dup() {
    push_(ir_t{ ir_t::IR_DUP });
}

void crapjit_t::emit_sink(uint32_t val) {
    push_(ir_t{ ir_t::IR_SINK, val*4 });
}

void crapjit_t::emit_getl(int32_t offs) {
    push_(ir_t{ ir_t::IR_GETL, uint32_t(offs*4) });
}

void crapjit_t::emit_setl(int32_t offs) {
    push_(ir_t{ ir_t::IR_SETL, uint32_t(offs*4) });
}

void crapjit_t::emit_frame(uint32_t offs) {
    push_(ir_t{ ir_t::IR_FRAME, uint32_t(offs*4) });
}

void crapjit_t::emit_return(uint32_t offs) {
    push_(ir_t{ ir_t::IR_RETURN, uint32_t(offs * 4) });
}

ir_t &crapjit_t::emit_call() {
    return push_(ir_t{ ir_t::IR_CALL });
}

ir_t& crapjit_t::emit_jz() {
    return push_(ir_t{ ir_t::IR_JZ });
}

ir_t& crapjit_t::emit_jnz() {
    return push_(ir_t{ ir_t::IR_JNZ });
}

ir_t& crapjit_t::emit_jmp() {
    return push_(ir_t{ ir_t::IR_JMP });
}

ir_t& crapjit_t::emit_label() {
    return push_(ir_t{ ir_t::IR_LABEL });
}

void crapjit_t::emit_add() {
    push_(ir_t{ ir_t::IR_ADD });
}

void crapjit_t::emit_sub() {
    push_(ir_t{ ir_t::IR_SUB });
}

void crapjit_t::emit_mul() {
    push_(ir_t{ ir_t::IR_MUL });
}

void crapjit_t::emit_and() {
    push_(ir_t{ ir_t::IR_AND });
}

void crapjit_t::emit_or() {
    push_(ir_t{ ir_t::IR_OR });
}

void crapjit_t::emit_notl() {
    push_(ir_t{ ir_t::IR_NOTL });
}

void crapjit_t::emit_lt() {
    push_(ir_t{ ir_t::IR_LT });
}

void crapjit_t::emit_leq() {
    push_(ir_t{ ir_t::IR_LEQ });
}

void crapjit_t::emit_gt() {
    push_(ir_t{ ir_t::IR_GT });
}

void crapjit_t::emit_geq() {
    push_(ir_t{ ir_t::IR_GEQ });
}

void crapjit_t::emit_eq() {
    push_(ir_t{ ir_t::IR_EQ });
}

void crapjit_t::emit_neq() {
    push_(ir_t{ ir_t::IR_NEQ });
}

ir_t& crapjit_t::push_(const ir_t& inst) {
    ir.push_back(new ir_t{ inst });
    return *(ir.back());
}

void* crapjit_t::finish() {

  std::map<const ir_t*, label_t> labels;
  std::map<const ir_t*, reloc_t> relocs;

  runasm::runasm_t x86{ code_, code_size_ };

  // emit instruction stream
  for (uint32_t index = 0; index < ir.size();) {

    ir_t* i = ir[index];

    if (i->type == ir_t::IR_LABEL) {

      x86.peepFence();

      labels[i] = x86.head();
      ++index;
      continue;
    }

    reloc_t reloc;
    const uint32_t used = codegen_(x86, index, reloc);
    assert(used >= 1);

    if (reloc.isValid()) {
      relocs[reloc.inst_] = reloc;
    }

    index += used;
  }

  // apply relocations
  for (const auto &i : relocs) {
    const ir_t *inst   = i.first;
    const ir_t *target = inst->target_;
    label_t ptr = labels[target];
    reloc_t rel = i.second;
    rel.set(ptr);
  }

  dumpCode_(x86.head());  // debug
  return code_;
}

void crapjit_t::dumpCode_(const uint8_t *until) {
  printf("\n\"");
  for (const uint8_t* i = code_; i < until; ++i) {
    printf("\\x%02x", *i);
  }
  printf("\"\n");
}

static inline runasm::cc_t instCc(const ir_t::type_t type) {
  using namespace runasm;
  switch (type) {
  case ir_t::IR_LT:  return CC_LT;
  case ir_t::IR_LEQ: return CC_LE;
  case ir_t::IR_GT:  return CC_GT;
  case ir_t::IR_GEQ: return CC_GE;
  case ir_t::IR_EQ:  return CC_EQ;
  case ir_t::IR_NEQ: return CC_NE;
  default:
    assert(!"unreachable");
    return cc_t(0);
  }
}

static inline runasm::cc_t negateCc(runasm::cc_t cc) {
  using namespace runasm;
  switch (cc) {
  case CC_LT: return CC_GE;
  case CC_LE: return CC_GT;
  case CC_GT: return CC_LE;
  case CC_GE: return CC_LT;
  case CC_EQ: return CC_NE;
  case CC_NE: return CC_EQ;
  default:
    assert(!"unreachable");
    return cc_t(0);
  }
}

uint32_t crapjit_t::codegenCmp_(runasm::runasm_t& x86, uint32_t index, reloc_t& reloc) {
  using namespace runasm;

  const ir_t& i = *ir[index];
  const uint32_t remain = ir.size() - index;

  if (remain >= 2) {
    const ir_t& j = *ir[index + 1];
    if (j.type == ir_t::IR_JNZ || j.type == ir_t::IR_JZ) {

      cc_t cc = instCc(i.type);
      if (j.type == ir_t::IR_JZ) {
        cc = negateCc(cc);
      }

      x86.POP(EAX);
      x86.POP(EDX);
      x86.CMP(EDX, EAX);
      rel32_t rel = x86.Jcc32(cc);
      reloc = reloc_t{ (uint8_t*)rel, reloc_t::RELOC_REL, &j };

      return 2;
    }
  }

  x86.POP (EAX);
  x86.POP (EDX);
  x86.CMP (EDX, EAX);
  x86.SET (instCc(i.type), EAX);
  x86.AND (EAX, 1u);
  x86.PUSH(EAX);
  return 1;
}

uint32_t crapjit_t::codegenConst_(runasm::runasm_t& x86, uint32_t index, reloc_t& reloc) {
  using namespace runasm;

  const ir_t& i = *ir[index];

  if (i.imm_ == 0) {
    x86.XOR(EAX, EAX);
  }
  else {
    x86.MOV(EAX, i.imm_);
  }
  x86.PUSH(EAX);

  return 1;
}

uint32_t crapjit_t::codegen_(runasm::runasm_t& x86, uint32_t index, reloc_t& reloc) {
  using namespace runasm;

  const ir_t& i = *ir[index];

  switch (i.type) {
  case ir_t::IR_LT:
  case ir_t::IR_LEQ:
  case ir_t::IR_GT:
  case ir_t::IR_GEQ:
  case ir_t::IR_EQ:
  case ir_t::IR_NEQ:
    return codegenCmp_(x86, index, reloc);

  case ir_t::IR_CONST:
    return codegenConst_(x86, index, reloc);

  case ir_t::IR_DROP:
    if (i.imm_) {
      x86.ADD(ESP, i.imm_);
    }
    return 1;

  case ir_t::IR_FRAME:
    x86.PUSH(EBP);
    x86.MOV(EBP, ESP);
    if (i.imm_) {
      x86.SUB(ESP, i.imm_);
    }
    return 1;

  case ir_t::IR_OR:
    x86.POP(EAX);
    x86.POP(EDX);
    x86.OR(EAX, EDX);
    x86.PUSH(EAX);
    return 1;

  case ir_t::IR_AND:
    x86.POP(EAX);
    x86.POP(EDX);
    x86.AND(EAX, EDX);
    x86.PUSH(EAX);
    return 1;

  case ir_t::IR_CALL: {
    rel32_t rel = x86.CALL();
    x86.PUSH(EAX);
    reloc = reloc_t{ (uint8_t*)rel, reloc_t::RELOC_REL, &i };
    return 1;
  }

  case ir_t::IR_RETURN:
    x86.POP(EAX);
    if (i.imm_) {
      x86.ADD(ESP, i.imm_);
    }
    x86.POP(EBP);
    x86.RET();
    return 1;

  case ir_t::IR_JMP: {
    rel32_t rel = x86.JMP32(label_t{});
    reloc = reloc_t{ (uint8_t*)rel, reloc_t::RELOC_REL, &i };
    return 1;
  }

  case ir_t::IR_JZ: {
    x86.POP(EAX);
    x86.CMP(EAX, 0u);
    rel32_t rel = x86.Jcc32(CC_EQ);
    reloc = reloc_t{ (uint8_t*)rel, reloc_t::RELOC_REL, &i };
    return 1;
  }

  case ir_t::IR_JNZ: {
    x86.POP(EAX);
    x86.CMP(EAX, 0u);
    rel32_t rel = x86.Jcc32(CC_NE);
    reloc = reloc_t{ (uint8_t*)rel, reloc_t::RELOC_REL, &i };
    return 1;
  }

  case ir_t::IR_SINK:
    if (i.imm_) {
      x86.POP(EAX);
      x86.ADD(ESP, i.imm_);
      x86.PUSH(EAX);
    }
    return 1;

  case ir_t::IR_NOTL:
    x86.POP(EAX);
    x86.TEST(EAX, EAX);
    x86.SET(CC_EQ, EAX);
    x86.AND(EAX, 1u);
    x86.PUSH(EAX);
    return 1;

  case ir_t::IR_DUP:
    x86.MOV(EAX, deref_t{ ESP });
    x86.PUSH(EAX);
    return 1;

  case ir_t::IR_ADD:
    x86.POP(EAX);
    x86.ADD(deref_t{ ESP }, EAX);
    return 1;

  case ir_t::IR_SUB:
    x86.POP(EAX);
    x86.SUB(deref_t{ ESP }, EAX);
    return 1;

  case ir_t::IR_MUL:
    x86.POP(EAX);
    x86.MOV(EDX, deref_t{ ESP });
    x86.IMUL(EDX);
    x86.MOV(deref_t{ ESP }, EAX);
    return 1;

  case ir_t::IR_GETL:
    // todo: push [ebp + imm]
    x86.MOV(EAX, sib_t{ 1, int32_t(i.imm_), EBP });
    x86.PUSH(EAX);
    return 1;

  case ir_t::IR_SETL:
    x86.POP(EAX);
    x86.MOV(sib_t{ 1, int32_t(i.imm_), EBP }, EAX);
    return 1;

  default:
    assert(!"unreachable");
    return 1;
  }
}

void reloc_t::set(label_t pos) {
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

}  // namespace cj
