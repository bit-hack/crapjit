#include <cstring>
#include <cassert>

#include "chunks.h"


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
    "\xb8\xdd\xcc\xbb\xaa"      // mov eax, 0xaabbccdd
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
    "\x58"                      // pop eax
    "\x5a"                      // pop edx
    "\x21\xd0"                  // and eax, edx
    "\x50"                      // push eax
  },
  { ins_or      , 5 , -1, -1,
    "\x58"                      // pop eax
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
    "\x81\xc4\xdd\xcc\xbb\xaa"  // sub esp, 0xaabbccdd
  },
  { ins_sink    , 8 ,  3, -1,
    "\x58"                      // pop eax
    "\x81\xc4\xdd\xcc\xbb\xaa"  // add esp, 0xaabbccdd
    "\x50"                      // push eax
  },
  { ins_frame   , 9 ,  5, -1,
    "\x55"                      // push ebp
    "\x89\xe5"                  // mov ebp, esp
    "\x81\xec\xdd\xcc\xbb\xaa"  // sub esp, 0xaabbccdd
  },
  { ins_ret     , 9 ,  3, -1, 
    "\x58"                      // pop eax
    "\x81\xc4\xdd\xcc\xbb\xaa"  // add esp, 0xaabbccdd
    "\x5d"                      // pop ebp
    "\xc3"                      // ret
  },
  { ins_call    , 6 , -1,  1,
    "\xe8\x39\xcc\xbb\xaa"      // call 0xaabbccdd
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
    "\xe9\x1f\xcc\xbb\xaa"      // jmp 0xaabbccdd
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

reloc_t chunk_emit(uint8_t*& ptr, jit_op op) {

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
