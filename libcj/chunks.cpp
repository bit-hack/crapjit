#include <cstring>
#include <cassert>
#include <initializer_list>

#include "chunks.h"
#include "runasm.h"


enum x86_r32_t {
  X86_EAX = 0,
  X86_ECX = 1,
  X86_EDX = 2,
  X86_EBX = 3,
  X86_ESP = 4,
  X86_EBP = 5,
  X86_ESI = 6,
  X86_EDI = 7,
};

enum x86_r8_t {
  X86_AL = 0,
  X86_CL = 1,
  X86_DL = 2,
  X86_BL = 3,
  X86_AH = 4,
  X86_CH = 5,
  X86_DH = 6,
  X86_BH = 7,
};

struct x86_t {

  x86_t(uint8_t* &ptr)
    : ptr(ptr)
  {
  }

  void emit(const uint32_t& imm) {
    const uint8_t out[] = {
      uint8_t(imm >> 0),
      uint8_t(imm >> 8),
      uint8_t(imm >> 16),
      uint8_t(imm >> 24)
    };
    emit(out, sizeof(out));
  }

  void emit(const uint8_t* data, size_t count) {
    memcpy(ptr, data, count);
    ptr += count;
  }

  void push(x86_r32_t reg) {
    const uint8_t out[] = { uint8_t(0x50 | reg) };
    emit(out, sizeof(out));
  }

  void pop(x86_r32_t reg) {
    const uint8_t out[] = { uint8_t(0x58 | reg) };
    emit(out, sizeof(out));
  }

  void setl(x86_r8_t reg) {
    const uint8_t out[] = { 0x0f, 0x9c, uint8_t(0xc0 | reg) };
    emit(out, sizeof(out));
  }

  void or (x86_r32_t dst, x86_r32_t src) {
    const uint8_t out[] = { 0x09, uint8_t(0b11000000 | (src << 3) | dst) };
    emit(out, sizeof(out));
  }

  void and(const x86_r32_t &dst, const x86_r32_t &src) {
    const uint8_t out[] = { 0x21, uint8_t(0b11000000 | (src << 3) | dst) };
    emit(out, sizeof(out));
  }

  void mov(x86_r32_t dst, const uint32_t &imm) {
    const uint8_t out[] = { 0xb8 };
    emit(out, sizeof(out));
    emit(imm);
  }

  void mov(const x86_r32_t &dst, const x86_r32_t &src) {
    const uint8_t out[] = { 0x89, uint8_t(0b11000000 | (src << 3) | dst) };
    emit(out, sizeof(out));
  }

  void ret() {
    const uint8_t out[] = { 0xc3 };
    emit(out, sizeof(out));
  }

  void sub(x86_r32_t dst, const uint32_t& imm) {
    // note: can we fit into 8 bit imm?
    const uint8_t out[] = { 0x81, uint8_t(0xe8 | dst) };
    emit(out, sizeof(out));
    emit(imm);
  }

  void add(x86_r32_t dst, const uint32_t& imm) {
    // note: can we fit into 8 bit imm?
    const uint8_t out[] = { 0x81, uint8_t(0xc0 | dst) };
    emit(out, sizeof(out));
    emit(imm);
  }

  void call(const uint32_t& imm) {
    const uint8_t out[] = { 0xe8 };
    emit(out, sizeof(out));
    emit(imm);
  }

  void jmp(const uint32_t& imm) {
    const uint8_t out[] = { 0xe9 };
    emit(out, sizeof(out));
    emit(imm);
  }

  uint8_t* &ptr;
};

namespace cj {

struct jit_chunk_t {
  jit_op   op_;
  uint32_t size_;
  int32_t  abs_op_;
  int32_t  rel_op_;
  const char* data_;
};

static const jit_chunk_t chunk_table[] = {
  { ins_const   , 6 ,  1, -1,
    "\xb8\xdd\xcc\xbb\xaa"      // mov eax, 0xaabbccdd            ###
    "\x50"                      // push eax
  },
  { ins_getl    , 7 ,  2, -1,
    "\x8b\x85\xdd\xcc\xbb\xaa"  // mov eax, [ebp+0xaabbccdd]
    "\x50"                      // push eax
  },
  { ins_setl    , 7 ,  3, -1,
    "\x58"                      // pop eax
    "\x89\x85\xdd\xcc\xbb\xaa"  // mov [ebp+0xaabbccdd], eax
  },
  { ins_add     , 4 , -1, -1,
    "\x58"                      // pop eax
    "\x01\x04\x24"              // add [esp], eax
  },
  { ins_sub     , 4 , -1, -1,
    "\x58"                      // pop eax
    "\x29\x04\x24"              // sub [esp], eax
  },
  { ins_mul     , 7 , -1, -1,
    "\x58"                      // pop eax
    "\xf7\x24\x24"              // mul dword [esp]
    "\x89\x04\x24"              // mov [esp], eax
  },
  { ins_div     , 7 , -1, -1,
    "\x58"                      // pop eax
    "\xf7\x34\x24"              // div dword [esp]
    "\x89\x04\x24"              // mov [esp], eax
  },
  { ins_lt      , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x9c\xc0"              // setl al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_lte     , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x9e\xc0"              // setl al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_gt      , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x9f\xc0"              // setg al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_gte     , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x9d\xc0"              // setge al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_eq      , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x94\xc0"              // sete al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_neq     , 11, -1, -1,
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x39\xc2"                  // cmp edx, eax
    "\x0f\x95\xc0"              // setne al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_and     , 5 , -1, -1,
    "\x58"                      // pop eax            ####
    "\x5a"                      // pop edx
    "\x21\xd0"                  // and eax, edx
    "\x50"                      // push eax
  },
  { ins_or      , 5 , -1, -1,
    "\x58"                      // pop eax            ####
    "\x5a"                      // pop edx
    "\x09\xd0"                  // or eax, edx
    "\x50"                      // push eax
  },
  { ins_notl    , 10, -1, -1,
    "\x58"                      // pop eax
    "\x85\xC0"                  // test eax, eax
    "\x0F\x94\xC0"              // setz al
    "\x83\xe0\x01"              // and eax, 1
    "\x50"                      // push eax
  },
  { ins_dup     , 4 , -1, -1,
    "\x8b\x04\x24"              // mov eax, [esp]
    "\x50"                      // push eax
  },
  { ins_drop    , 6 ,  2, -1,
    "\x81\xc4\xdd\xcc\xbb\xaa"  // add esp, 0xaabbccdd    ###
  },
  { ins_sink    , 8 ,  3, -1,
    "\x58"                      // pop eax
    "\x81\xc4\xdd\xcc\xbb\xaa"  // add esp, 0xaabbccdd
    "\x50"                      // push eax
  },
  { ins_frame   , 9 ,  5, -1,
    "\x55"                      // push ebp               ###
    "\x89\xe5"                  // mov ebp, esp
    "\x81\xec\xdd\xcc\xbb\xaa"  // sub esp, 0xaabbccdd
  },
  { ins_ret     , 9 ,  3, -1, 
    "\x58"                      // pop eax                ###
    "\x81\xc4\xdd\xcc\xbb\xaa"  // add esp, 0xaabbccdd
    "\x5d"                      // pop ebp
    "\xc3"                      // ret
  },
  { ins_call    , 6 , -1,  1,
    "\xe8\x39\xcc\xbb\xaa"      // call 0xaabbccdd        ###
    "\x50"                      // push eax
  },
  { ins_jnz     , 10, -1,  6,
    "\x58"                      // pop eax
    "\x83\xf8\x00"              // cmp eax, 0
    "\x0f\x85\x2e\xcc\xbb\xaa"  // jnz 0xaabbccdd
  },
  { ins_jz      , 10, -1,  6,
    "\x58"                      // pop eax
    "\x83\xf8\x00"              // cmp eax, 0
    "\x0f\x84\x24\xcc\xbb\xaa"  // jz 0xaabbccdd
  },
  { ins_jmp     , 5 , -1,  1,
    "\xe9\x1f\xcc\xbb\xaa"      // jmp 0xaabbccdd         ###
  },
};

static void insert(uint8_t*& ptr, const void* data, const uint32_t size) {
  memcpy(ptr, data, size);
  ptr += size;
}

template <typename type_t>
static void insert(uint8_t*& ptr, const type_t val) {
  const uint32_t size = sizeof(type_t);
  memcpy(ptr, &val, size);
  ptr += size;
}

static reloc_t chunk_emit_table(uint8_t*& ptr, jit_op op) {

  const jit_chunk_t& chunk = chunk_table[op];

  reloc_t reloc;

  uint8_t* base = ptr;

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

reloc_t chunk_emit(uint8_t*& ptr, jit_op op) {

  x86_t x86{ ptr };

  switch (op) {
  case ins_const:
    x86.mov (X86_EAX, 0xaabbccdd);
    x86.push(X86_EAX);
    return reloc_t{ ptr - 5, reloc_t::RELOC_ABS };

  case ins_drop:
    x86.add(X86_ESP, 0xaabbccdd);
    return reloc_t{ ptr - 4, reloc_t::RELOC_ABS };

  case ins_frame:
    x86.push(X86_EBP);
    x86.mov (X86_EBP, X86_ESP);
    x86.sub (X86_ESP, 0xaabbccdd);
    return reloc_t{ ptr - 4, reloc_t::RELOC_ABS };

  case ins_or:
    x86.pop (X86_EAX);
    x86.pop (X86_EDX);
    x86.or  (X86_EAX, X86_EDX);
    x86.push(X86_EAX);
    return reloc_t{};

  case ins_and:
    x86.pop (X86_EAX);
    x86.pop (X86_EDX);
    x86.and (X86_EAX, X86_EDX);
    x86.push(X86_EAX);
    return reloc_t{};

  case ins_call:
    x86.call(0xaabbccdd);
    x86.push(X86_EAX);
    return reloc_t{ ptr - 5, reloc_t::RELOC_REL };

  case ins_ret:
    x86.pop(X86_EAX);
    x86.add(X86_ESP, 0xaabbccdd);
    x86.pop(X86_EBP);
    x86.ret();
    return reloc_t{ ptr - 6, reloc_t::RELOC_ABS };

  case ins_jmp:
    x86.jmp(0xaabbccdd);
    return reloc_t{ ptr - 4, reloc_t::RELOC_REL };
  }

  return chunk_emit_table(ptr, op);
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

}  // namespace cj
