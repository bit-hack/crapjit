#include "chunks.h"

namespace cj {

jit_chunk_t chunk_table[] = {
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
  { ins_lte     , 11, -1, -1, "\x58\x5a\x39\xc2\x0f\x9e\xc0\x83\xe0\x01\x50" },
  { ins_gt      , 11, -1, -1, "\x58\x5a\x39\xc2\x0f\x9f\xc0\x83\xe0\x01\x50" },
  { ins_gte     , 11, -1, -1, "\x58\x5a\x39\xc2\x0f\x9d\xc0\x83\xe0\x01\x50" },
  { ins_eq      , 11, -1, -1, "\x58\x5a\x39\xc2\x0f\x94\xc0\x83\xe0\x01\x50" },
  { ins_neq     , 11, -1, -1, "\x58\x5a\x39\xc2\x0f\x95\xc0\x83\xe0\x01\x50" },
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
  { ins_notl    , 5 , -1, -1,
    "\x58"                      // pop eax
    "\x83\xf0\x01"              // xor eax, 1
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

}  // namespace cj
