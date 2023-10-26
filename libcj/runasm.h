#pragma once

//
// Small X86 dynarec engine
//   github.com/bit-hack
//
// Based on runasm_t definitions v0.5.1 by:
//   linuzappz <linuzappz@pcsx.net>
//   alexey silinov
//
// Nice x86 reference at:
//   https://www.felixcloutier.com/x86/index.html
//

#include <cassert>
#include <cstdint>
#include <initializer_list>


namespace runasm {

// Calling Conventions
//
//  Name                  __cdecl                 __stdcall
//  Arg Order             Right->Left             Right->Left
//  Caller Save           eax ecx edx             eax ecx edx
//  Callee Save           ebx esp ebp esi edi     ebx esp ebp esi edi
//  Cleanup               caller                  callee
//  Return value          eax                     eax

// 8bit regs
enum gp_reg8_t {
  AL = 0,
  CL = 1,
  DL = 2,
  BL = 3,
  AH = 4,
  CH = 5,
  DH = 6,
  BH = 7,
};

// 16 bit regs
enum gp_reg16_t {
  AX = 0,
  CX = 1,
  DX = 2,
  BX = 3,
  SP = 4,
  BP = 5,
  SI = 6,
  DI = 7,
};

// 32 bit regs
enum gp_reg32_t {
  EAX = 0,
  ECX = 1,
  EDX = 2,
  EBX = 3,
  ESP = 4,
  EBP = 5,
  ESI = 6,
  EDI = 7,
};

enum cc_t {
  CC_O  = 0x0, // overflow         JO    (OF=1)
  CC_NO = 0x1, // not overflow     JNO   (OF=0)
  CC_C  = 0x2, // carry            JC    (CF=1)
  CC_AE = 0x3, // above or equal   JAE   (CF=0)
  CC_EQ = 0x4, // equal            JE    (ZF=1)
  CC_NE = 0x5, // not equal        JNE   (ZF=0)
  CC_BE = 0x6, // below or equal   JBE   (CF=1 or ZF=1)
  CC_AB = 0x7, // above            JA    (CF=0 and ZF=0)
  CC_S  = 0x8, // sign             JS    (SF=1)
  CC_NS = 0x9, // not sign         JNS   (SF=0)
  CC_P  = 0xa, // parity           JP    (PF=1)
  CC_NP = 0xb, // parity odd       JNP   (PF=0)
  CC_LT = 0xc, // less             JL    (SF!=OF)
  CC_GE = 0xd, // greater or equal JGE   (SF=OF)
  CC_LE = 0xe, // less or equal    JLE   (ZF=1 or SF!=OF)
  CC_GT = 0xf, // greater          JG    (ZF=0 and SF=OF)
};

// helper types
typedef uint8_t  *rel8_t;
typedef uint32_t *rel32_t;
typedef uint8_t  *mem8_t;
typedef uint16_t *mem16_t;
typedef uint32_t *mem32_t;

struct label_t {

  label_t() : ptr(nullptr) {
  }

  label_t(void *p) : ptr(p) {
  }

  operator bool() const {
    return ptr != nullptr;
  }

  void * const ptr;
};

struct deref_t {

  deref_t(gp_reg32_t r)
    : reg(r)
    , disp32(0)
    , type(type_reg) {
  }

  deref_t(gp_reg32_t r, int32_t disp32)
    : reg(r)
    , disp32(disp32)
    , type(type_reg) {
  }

  deref_t(mem32_t m)
    : mem(m)
    , type(type_mem) {
  }

  union {
    gp_reg32_t reg;
    mem32_t mem;
  };
  int32_t disp32;

  bool is_reg() const {
    return type == type_reg;
  }

  bool is_mem() const {
    return type == type_mem;
  }

protected:
  enum type_t {
    type_reg,
    type_mem,
  };
  const type_t type;
};

struct sib_t {

  explicit sib_t(uint32_t scale, int32_t disp, gp_reg32_t base)
      : b(base), i(ESP), disp(disp), is_disp(true) {
    switch (scale) {
    case 1: s = 0; break;
    case 2: s = 1; break;
    case 4: s = 2; break;
    case 8: s = 3; break;
    default: assert(!"invalid scale value");
    }
  }

  explicit sib_t(uint32_t scale, gp_reg32_t index, gp_reg32_t base)
      : i(index), b(base), is_disp(false) {
    assert(index != ESP);
    switch (scale) {
    case 1: s = 0; break;
    case 2: s = 1; break;
    case 4: s = 2; break;
    case 8: s = 3; break;
    default: assert(!"invalid scale value");
    }
  }

  gp_reg32_t i;
  gp_reg32_t b;
  int32_t disp;
  bool is_disp;
  uint8_t s;
};

struct runasm_t {

  // construct with target code buffer
  runasm_t(uint8_t *dst, size_t size)
    : start   (dst)
    , ptr     (dst)
    , end     (dst + size)
    , peepCeil(dst)
  {
    assert(dst);
  }

  // return pointer to the code buffer
  uint8_t *code() const {
    return start;
  }

  uint8_t* head() const {
    return ptr;
  }

  // reset the instruction stream
  void reset() {
    ptr = start;
  }

  // write value to code buffer
  void write8 (const uint8_t val);
  void write16(const uint16_t val);
  void write32(const uint32_t val);
  void write32(void *val);
  void write  (const void *data, size_t count);

  // set jump target
  void setTarget(rel8_t op);
  void setTarget(rel32_t op);
  void setTarget(rel8_t op, label_t target);
  void setTarget(rel32_t op, label_t target);
  void setTarget(std::initializer_list<rel32_t> op, label_t target);
  void setTarget(std::initializer_list<rel8_t> op, label_t target);

  // align instruction stream
  void align(int32_t bytes);

  // return a label at this point in the code stream
  label_t label() const;

  // --------------------------------------------------------------------------
  // mov instructions
  // --------------------------------------------------------------------------

  // mov r32, r32
  void MOV(gp_reg32_t dst, gp_reg32_t src);
  // mov [m32], r32
  void MOV(mem32_t dst, gp_reg32_t src);
  // mov r32, [m32]
  void MOV(gp_reg32_t dst, mem32_t src);
  // mov r32, [r32 + disp32]
  void MOV(gp_reg32_t dst, deref_t src);
  // mov [r32 + disp32], r32
  void MOV(deref_t dst, gp_reg32_t src);
  // mov r32, imm32
  void MOV(gp_reg32_t dst, uint32_t src);
  // mov [m32], imm32
  void MOV(mem32_t dst, uint32_t src);

  // mov r32, [base + scale*index]
  void MOV(gp_reg32_t dst, sib_t src);
  // mov [base + scale*index], r32
  void MOV(sib_t dst, gp_reg32_t src);

  // mov r16 to m16
  void MOV(mem16_t dst, gp_reg16_t src);
  // mov m16 to r16
  void MOV(gp_reg16_t dst, mem16_t src);
  // mov imm16 to m16
  void MOV(mem16_t dst, uint16_t src);

  // mov r8 to m8
  void MOV(mem8_t dst, gp_reg8_t src);
  // mov m8 to r8
  void MOV(gp_reg8_t dst, mem8_t src);
  // mov imm8 to m8
  void MOV(mem8_t dst, uint8_t src);

  // --------------------------------------------------------------------------
  // mov sign extend
  // --------------------------------------------------------------------------

  // mov sign extend r8 to r32
  void MOVSX(gp_reg32_t dst, gp_reg8_t src);
  // mov sign extend m8 to r32
  void MOVSX(gp_reg32_t dst, mem8_t src);
  // mov sign extend r16 to r32
  void MOVSX(gp_reg32_t dst, gp_reg16_t src);
  // mov sign extend m16 to r32
  void MOVSX(gp_reg32_t dst, mem16_t src);

  // --------------------------------------------------------------------------
  // mov zero extend
  // --------------------------------------------------------------------------

  // mov zero extend r8 to r32
  void MOVZX(gp_reg32_t dst, gp_reg8_t src);
  // mov zero extend m8 to r32
  void MOVZX(gp_reg32_t dst, mem8_t src);
  // mov zero extend r16 to r32
  void MOVZX(gp_reg32_t dst, gp_reg16_t src);
  // mov zero extend m16 to r32
  void MOVZX(gp_reg32_t dst, mem16_t src);

  // --------------------------------------------------------------------------
  // conditional move instructions
  // --------------------------------------------------------------------------

  // conditional move
  void CMOV(cc_t cc, gp_reg32_t dst, gp_reg32_t src);
  // conditional move
  void CMOV(cc_t cc, gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // addition
  // --------------------------------------------------------------------------

  // add imm32 to r32
  void ADD(gp_reg32_t dst, uint32_t src);
  // add imm32 to m32
  void ADD(mem32_t dst, uint32_t src);
  // add r32 to r32
  void ADD(gp_reg32_t dst, gp_reg32_t src);
  // add r32 to m32
  void ADD(mem32_t dst, gp_reg32_t src);
  // add m32 to r32
  void ADD(gp_reg32_t dst, mem32_t src);
  // add [r32 + disp32], r32
  void ADD(deref_t dst, gp_reg32_t src);

  // --------------------------------------------------------------------------
  // add with carry
  // --------------------------------------------------------------------------

  // adc imm32 to r32
  void ADC(gp_reg32_t dst, uint32_t src);
  // adc r32 to r32
  void ADC(gp_reg32_t dst, gp_reg32_t src);
  // adc m32 to r32
  void ADC(gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // increment
  // --------------------------------------------------------------------------

  // inc r32
  void INC(gp_reg32_t dst);
  // inc m32
  void INC(mem32_t dst);

  // --------------------------------------------------------------------------
  // subtract
  // --------------------------------------------------------------------------

  // sub imm32 to r32
  void SUB(gp_reg32_t dst, uint32_t src);
  // sub r32 to r32
  void SUB(gp_reg32_t dst, gp_reg32_t src);
  // sub m32 to r32
  void SUB(gp_reg32_t dst, mem32_t src);
  // sub [r32 + disp32], r32
  void SUB(deref_t dst, gp_reg32_t src);

  // --------------------------------------------------------------------------
  // subtract with borrow
  // --------------------------------------------------------------------------

  // sbb imm32 to r32
  void SBB(gp_reg32_t dst, uint32_t src);
  // sbb r32 to r32
  void SBB(gp_reg32_t dst, gp_reg32_t src);
  // sbb m32 to r32
  void SBB(gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // decrement
  // --------------------------------------------------------------------------

  // dec r32
  void DEC(gp_reg32_t dst);
  // dec m32
  void DEC(mem32_t dst);

  // --------------------------------------------------------------------------
  // multiply
  // --------------------------------------------------------------------------

  // mul eax by r32 to edx:eax
  void MUL(gp_reg32_t src);
  // mul eax by m32 to edx:eax
  void MUL(mem32_t src);

  // --------------------------------------------------------------------------
  // integer multiply
  // --------------------------------------------------------------------------

  // imul eax by r32 to edx:eax
  void IMUL(gp_reg32_t src);
  // imul eax by m32 to edx:eax
  void IMUL(mem32_t src);
  // imul r32 by r32 to r32
  void IMUL(gp_reg32_t dst, gp_reg32_t src);

  // --------------------------------------------------------------------------
  // divide
  // --------------------------------------------------------------------------

  // div eax by r32 to edx:eax
  void DIV(gp_reg32_t src);
  // div eax by m32 to edx:eax
  void DIV(mem32_t src);

  // --------------------------------------------------------------------------
  // integer divide
  // --------------------------------------------------------------------------

  // idiv eax by r32 to edx:eax
  void IDIV(gp_reg32_t src);
  // idiv eax by m32 to edx:eax
  void IDIV(mem32_t src);

  // --------------------------------------------------------------------------
  // rotate carry right
  // --------------------------------------------------------------------------

  // rotate carry right
  void RCR(int32_t dst, int32_t src);

  // --------------------------------------------------------------------------
  // shift lefy
  // --------------------------------------------------------------------------

  // shl imm8 to r32
  void SHL(gp_reg32_t dst, uint8_t src);
  // shl cl to r32
  void SHL(gp_reg32_t dst);

  // --------------------------------------------------------------------------
  // shift right
  // --------------------------------------------------------------------------

  // shr imm8 to r32
  void SHR(gp_reg32_t dst, uint8_t src);
  // shr cl to r32
  void SHR(gp_reg32_t dst);

  // --------------------------------------------------------------------------
  // shift right arithmetic
  // --------------------------------------------------------------------------

  // sar imm8 to r32
  void SAR(gp_reg32_t dst, uint8_t src);
  // sar cl to r32
  void SAR(gp_reg32_t dst);

  // --------------------------------------------------------------------------
  // bitwise or
  // --------------------------------------------------------------------------

  // or imm32 to r32
  void OR(gp_reg32_t dst, uint32_t src);
  // or imm32 to m32
  void OR(mem32_t dst, uint32_t src);
  // or r32 to r32
  void OR(gp_reg32_t dst, gp_reg32_t src);
  // or r32 to m32
  void OR(mem32_t dst, gp_reg32_t src);
  // or m32 to r32
  void OR(gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // bitwise exclusive or
  // --------------------------------------------------------------------------

  // xor imm32 to r32
  void XOR(gp_reg32_t dst, uint32_t src);
  // xor imm32 to m32
  void XOR(mem32_t dst, uint32_t src);
  // xor r32 to r32
  void XOR(gp_reg32_t dst, gp_reg32_t src);
  // xor r32 to m32
  void XOR(mem32_t dst, gp_reg32_t src);
  // xor m32 to r32
  void XOR(gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // bitwise and
  // --------------------------------------------------------------------------

  // and imm32 to r32
  void AND(gp_reg32_t dst, uint32_t src);
  // and imm32 to m32
  void AND(mem32_t dst, uint32_t src);
  // and r32 to r32
  void AND(gp_reg32_t dst, gp_reg32_t src);
  // and r32 to m32
  void AND(mem32_t dst, gp_reg32_t src);
  // and m32 to r32
  void AND(gp_reg32_t dst, mem32_t src);

  // --------------------------------------------------------------------------
  // bitwise not
  // --------------------------------------------------------------------------

  // not r32
  void NOT(gp_reg32_t src);
  // neg r32
  void NEG(gp_reg32_t src);

  // --------------------------------------------------------------------------
  // no operation
  // --------------------------------------------------------------------------

  void NOP();

  // --------------------------------------------------------------------------
  // debug breakpoint
  // --------------------------------------------------------------------------

  void INT3();

  // --------------------------------------------------------------------------
  // conditional jump
  // --------------------------------------------------------------------------

  rel8_t  Jcc8 (cc_t cc, label_t to = label_t());
  rel32_t Jcc32(cc_t cc, label_t to = label_t());

  // --------------------------------------------------------------------------
  // unconditional jump
  // --------------------------------------------------------------------------

  // jmp rel8
  rel8_t JMP8(label_t to = label_t());
  // jmp rel32
  rel32_t JMP32(label_t to = label_t());
  // jmp r32
  void JMP(gp_reg32_t to);

  // --------------------------------------------------------------------------
  // procedure call
  // --------------------------------------------------------------------------

  // call near, relative, displacement relative to next instruction
  void CALL(void *func);
  // call near, relative, displacement relative to next instruction
  rel32_t CALL(label_t to = label_t());
  // call near, relative, displacement relative to next instruction
  void CALL(gp_reg32_t to);
  // call far, absolute indirect, address given in m16:32
  // XXX: (mem32_t *to)
  void CALL_32M(void *to);

  // --------------------------------------------------------------------------
  // bit test
  // --------------------------------------------------------------------------

  void BT(gp_reg32_t dst, int32_t src);

  // --------------------------------------------------------------------------
  // compare
  // --------------------------------------------------------------------------

  // cmp imm32 to r32
  void CMP(gp_reg32_t lhs, uint32_t rhs);
  // cmp imm32 to m32
  void CMP(mem32_t lhs, uint32_t rhs);
  // cmp r32 to r32
  void CMP(gp_reg32_t lhs, gp_reg32_t rhs);
  // cmp m32 to r32
  void CMP(gp_reg32_t lhs, mem32_t rhs);

  // --------------------------------------------------------------------------
  // test
  // --------------------------------------------------------------------------

  // test imm32 to r32
  void TEST(gp_reg32_t dst, uint32_t src);
  // test r32 to r32
  void TEST(gp_reg32_t dst, gp_reg32_t src);

  // --------------------------------------------------------------------------
  // conditional set
  // --------------------------------------------------------------------------

  // set byte on condition
  void SET(cc_t cc, gp_reg32_t dst);

  // --------------------------------------------------------------------------
  // data conversion
  // --------------------------------------------------------------------------

  // convert byte to word
  void CBW();
  // convert word to doubleword
  void CWD();
  // convert doubleword to quadword
  void CDQ();

  // --------------------------------------------------------------------------
  // stack push
  // --------------------------------------------------------------------------

  // push r32 to stack
  void PUSH(gp_reg32_t src);
  // push m32 to stack
  void PUSH(mem32_t src);
  // push imm32 to stack
  void PUSH(uint32_t src);

  // push All General-Purpose Registers
  void PUSHA();

  // --------------------------------------------------------------------------
  // stack pop
  // --------------------------------------------------------------------------

  // pop r32 from stack
  void POP(gp_reg32_t src);

  // pop All General-Purpose Registers
  void POPA();

  // --------------------------------------------------------------------------
  // return from procedure
  // --------------------------------------------------------------------------

  // return
  void RET();

  // --------------------------------------------------------------------------
  // peephole optimization related
  // --------------------------------------------------------------------------

  void peepFence() {
    // branch targets should not be optimized across
    // this sets a new boundary for the peephole optimizer
    peepCeil = ptr;
  }

protected:
  void modRM(int32_t mod, int32_t rm, int32_t reg);
  void modRMSibSB(int32_t reg, const sib_t &sib);

  void postEmit();

  uint32_t prior32(uint32_t index) const {
    return *(uint32_t*)(ptr-index);
  }

  uint8_t prior8(uint32_t index) const {
    return *(uint8_t*)(ptr-index);
  }

  uint8_t *priorPtr(uint32_t back) const {
    return (uint8_t*)(ptr-back);
  }

  uint8_t *start;
  uint8_t *ptr;
  const uint8_t *end;

  uint8_t* peepCeil;

}; // struct runasm_t

} // namespace runasm
