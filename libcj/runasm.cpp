#include <cassert>

#include "runasm.h"

namespace runasm {

enum {
  SIB = 4,
  DISP32 = 5,
};

label_t runasm_t::label() const {
  return ptr;
}

void runasm_t::write8(const uint8_t val) {
  assert((end - ptr) >= 1);
  *(uint8_t *)ptr = val;
  ptr += 1;
}

void runasm_t::write16(const uint16_t val) {
  assert((end - ptr) >= 2);
  *(uint16_t *)ptr = val;
  ptr += 2;
}

void runasm_t::write32(const uint32_t val) {
  assert((end - ptr) >= 4);
  *(uint32_t *)ptr = val;
  ptr += 4;
}

void runasm_t::write32(void *val) {
  assert((end - ptr) >= 4);
  *(uint32_t *)ptr = uint32_t(val);
  ptr += 4;
}

void runasm_t::setTarget(rel8_t j8, label_t target) {
  uint32_t jump = ((uint32_t)target.ptr - (uint32_t)j8) - 1;
  assert(jump <= 0x7f);
  *j8 = (uint8_t)jump;
}

void runasm_t::setTarget(rel32_t j32, label_t target) {
  *j32 = ((uint32_t)target.ptr - (uint32_t)j32) - 4;
}

void runasm_t::setTarget(rel8_t j8) {
  uint32_t jump = (ptr - (int8_t *)j8) - 1;
  assert(jump <= 0x7f);
  *j8 = (uint8_t)jump;
}

void runasm_t::setTarget(rel32_t j32) {
  *j32 = (ptr - (int8_t *)j32) - 4;
}

void runasm_t::setTarget(std::initializer_list<rel32_t> branches,
                         label_t target) {
  for (auto &j32 : branches) {
    *j32 = ((uint32_t)target.ptr - (uint32_t)j32) - 4;
  }
}

void runasm_t::setTarget(std::initializer_list<rel8_t> branches,
                         label_t target) {
  for (auto &j8 : branches) {
    uint32_t jump = ((uint32_t)target.ptr - (uint32_t)j8) - 1;
    assert(jump <= 0x7f);
    *j8 = (uint8_t)jump;
  }
}

void runasm_t::align(int32_t bytes) {
  ptr = (int8_t *)(((uintptr_t)ptr + bytes) & ~(bytes - 1));
  assert(ptr <= end);
}

void runasm_t::modRM(int32_t mod, int32_t rm, int32_t reg) {
  write8((mod << 6) | (rm << 3) | reg);
}

void runasm_t::modRMSibSB(int32_t rm, const sib_t &sib) {

  //   mod  reg  r/m
  // 
  //   00   100  ...   SIB
  //   01   100  ...   SIB + disp8
  //   10   100  ...   SIB + disp32

  const int8_t reg  = 4;
  const int8_t mod = sib.is_disp ? ((sib.disp & 0xffffff00) ? 2 : 1) : 0;

  modRM(mod, rm, reg);

  // scale  76
  // index    543
  // base        210
  write8((sib.s << 6) | (sib.i << 3) | (sib.b));

  // displacement
  switch (mod) {
  case 1:
    write8(sib.disp & 0xff);
    break;
  case 2:
    write32(sib.disp);
    break;
  }
}

// mov instructions

void runasm_t::MOV(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x89);
  modRM(3, src, dst);
}

void runasm_t::MOV(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x89);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::MOV(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x8B);
  modRM(0, dst, DISP32);
  write32(src);
}

void runasm_t::MOV(gp_reg32_t dst,
                   deref_t src) {
  write8(0x8B);
  assert(src.is_reg());
  if (src.reg == ESP) {
    // special case with SIB byte
    modRMSibSB(dst, sib_t(1, src.disp32, src.reg));
  }
  else {
    if (src.disp32) {
      modRM(2, dst, src.reg);
      write32(src.disp32);
    } else {
      modRM(0, dst, src.reg);
    }
  }
}

void runasm_t::MOV(gp_reg32_t dst, sib_t src) {
  write8(0x8B);
  modRMSibSB(dst, src);
}

void runasm_t::MOV(deref_t dst,
                   gp_reg32_t src) {
  write8(0x89);
  assert(dst.is_reg());
  if (dst.reg == ESP) {
    // special case with SIB byte
    modRMSibSB(src, sib_t(1, dst.disp32, dst.reg));
  }
  else {
    if (dst.disp32) {
      modRM(2, src, dst.reg);
      write32(dst.disp32);
    } else {
      modRM(0, src, dst.reg);
    }
  }
}

void runasm_t::MOV(sib_t dst,
                   gp_reg32_t src) {
  write8(0x89);
  modRMSibSB(src, dst);
}

void runasm_t::MOV(gp_reg32_t dst, uint32_t src) {
  write8(0xB8 | dst);
  write32(src);
}

void runasm_t::MOV(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0xC7);
  modRM(0, 0, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::MOV(mem16_t dst, gp_reg16_t src) {
  assert(dst);
  write8(0x66);
  write8(0x89);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::MOV(gp_reg16_t dst, mem16_t src) {
  assert(src);
  write8(0x66);
  write8(0x8B);
  modRM(0, dst, DISP32);
  write32(src);
}

void runasm_t::MOV(mem16_t dst, uint16_t src) {
  assert(dst);
  write8(0x66);
  write8(0xC7);
  modRM(0, 0, DISP32);
  write32(dst);
  write16(src);
}

void runasm_t::MOV(mem8_t dst, gp_reg8_t src) {
  assert(dst);
  write8(0x88);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::MOV(gp_reg8_t dst, mem8_t src) {
  assert(src);
  write8(0x8A);
  modRM(0, dst, DISP32);
  write32(src);
}

void runasm_t::MOV(mem8_t dst, uint8_t src) {
  assert(dst);
  assert(src);
  write8(0xC6);
  modRM(0, 0, DISP32);
  write32(dst);
  write8(src);
}

// mov sign extend

void runasm_t::MOVSX(gp_reg32_t dst, gp_reg8_t src) {
  write16(0xBE0F);
  modRM(3, dst, src);
}

void runasm_t::MOVSX(gp_reg32_t dst, mem8_t src) {
  assert(src);
  write16(0xBE0F);
  modRM(0, dst, DISP32);
  write32(src);
}

void runasm_t::MOVSX(gp_reg32_t dst, gp_reg16_t src) {
  write16(0xBF0F);
  modRM(3, dst, src);
}

void runasm_t::MOVSX(gp_reg32_t dst, mem16_t src) {
  assert(src);
  write16(0xBF0F);
  modRM(0, dst, DISP32);
  write32(src);
}

// mov zero extend

void runasm_t::MOVZX(gp_reg32_t dst, gp_reg8_t src) {
  write16(0xB60F);
  modRM(3, dst, src);
}

void runasm_t::MOVZX(gp_reg32_t dst, mem8_t src) {
  assert(src);
  write16(0xB60F);
  modRM(0, dst, DISP32);
  write32(src);
}

void runasm_t::MOVZX(gp_reg32_t dst, gp_reg16_t src) {
  write16(0xB70F);
  modRM(3, dst, src);
}

void runasm_t::MOVZX(gp_reg32_t dst, mem16_t src) {
  assert(src);
  write16(0xB70F);
  modRM(0, dst, DISP32);
  write32(src);
}

// conditional move instructions

void runasm_t::CMOV(cc_t cc, gp_reg32_t dst, gp_reg32_t src) {
  write8(0x0F);
  write8(0x40 | cc);
  modRM(3, dst, src);
}

void runasm_t::CMOV(cc_t cc, gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x0F);
  write8(0x40 | cc);
  modRM(0, dst, DISP32);
  write32(src);
}

// add instructions

void runasm_t::ADD(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x05);
  } else {
    write8(0x81);
    modRM(3, 0, dst);
  }
  write32(src);
}

void runasm_t::ADD(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 0, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::ADD(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x01);
  modRM(3, src, dst);
}

void runasm_t::ADD(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x01);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::ADD(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x03);
  modRM(0, dst, DISP32);
  write32(src);
}

// add with carry instructions

void runasm_t::ADC(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x15);
  } else {
    write8(0x81);
    modRM(3, 2, dst);
  }
  write32(src);
}

void runasm_t::ADC(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x11);
  modRM(3, src, dst);
}

void runasm_t::ADC(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x13);
  modRM(0, dst, DISP32);
  write32(src);
}

// increment instructions

void runasm_t::INC(gp_reg32_t dst) {
  write8(0x40 + dst);
}

void runasm_t::INC(mem32_t dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 0, DISP32);
  write32(dst);
}

// subtract instructions

void runasm_t::SUB(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x2D);
  } else {
    write8(0x81);
    modRM(3, 5, dst);
  }
  write32(src);
}

void runasm_t::SUB(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x29);
  modRM(3, src, dst);
}

void runasm_t::SUB(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x2B);
  modRM(0, dst, DISP32);
  write32(src);
}

// subtract with borrow instructions

void runasm_t::SBB(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x1D);
  } else {
    write8(0x81);
    modRM(3, 3, dst);
  }
  write32(src);
}

void runasm_t::SBB(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x19);
  modRM(3, src, dst);
}

void runasm_t::SBB(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x1B);
  modRM(0, dst, DISP32);
  write32(src);
}

// decrement instructions

void runasm_t::DEC(gp_reg32_t dst) {
  write8(0x48 + dst);
}

void runasm_t::DEC(mem32_t dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 1, DISP32);
  write32(dst);
}

// multiply instructions

void runasm_t::MUL(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 4, src);
}

void runasm_t::MUL(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 4, DISP32);
  write32(src);
}

// integer multiply instructions

void runasm_t::IMUL(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 5, src);
}

void runasm_t::IMUL(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 5, DISP32);
  write32(src);
}

void runasm_t::IMUL(gp_reg32_t dst, gp_reg32_t src) {
  write16(0xAF0F);
  modRM(3, dst, src);
}

// divide instructions

void runasm_t::DIV(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 6, src);
}

void runasm_t::DIV(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 6, DISP32);
  write32(src);
}

// integer divide instructions

void runasm_t::IDIV(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 7, src);
}

void runasm_t::IDIV(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 7, DISP32);
  write32(src);
}

// rotate carry right instructions

void runasm_t::RCR(int32_t dst, int32_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xd8 | dst);
  } else {
    write8(0xc1);
    write8(0xd8 | dst);
    write8(src);
  }
}

// shift left instructions

void runasm_t::SHL(gp_reg32_t dst, uint8_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xe0 | dst);
    return;
  }
  write8(0xC1);
  modRM(3, 4, dst);
  write8(src);
}

void runasm_t::SHL(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 4, dst);
}

// shift right instructions

void runasm_t::SHR(gp_reg32_t dst, uint8_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xe8 | dst);
    return;
  }
  write8(0xC1);
  modRM(3, 5, dst);
  write8(src);
}

void runasm_t::SHR(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 5, dst);
}

// shift arithmetic right instructions

void runasm_t::SAR(gp_reg32_t dst, uint8_t src) {
  write8(0xC1);
  modRM(3, 7, dst);
  write8(src);
}

void runasm_t::SAR(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 7, dst);
}

// bitwise or instructions

void runasm_t::OR(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x0D);
  } else {
    write8(0x81);
    modRM(3, 1, dst);
  }
  write32(src);
}

void runasm_t::OR(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 1, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::OR(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x09);
  modRM(3, src, dst);
}

void runasm_t::OR(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x09);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::OR(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x0B);
  modRM(0, dst, DISP32);
  write32(src);
}

// bitwise xor instructions

void runasm_t::XOR(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x35);
  } else {
    write8(0x81);
    modRM(3, 6, dst);
  }
  write32(src);
}

void runasm_t::XOR(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 6, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::XOR(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x31);
  modRM(3, src, dst);
}

void runasm_t::XOR(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x31);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::XOR(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x33);
  modRM(0, dst, DISP32);
  write32(src);
}

// bitwise and instructions

void runasm_t::AND(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x25);
  } else {
    write8(0x81);
    modRM(3, 0x4, dst);
  }
  write32(src);
}

void runasm_t::AND(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 0x4, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::AND(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x21);
  modRM(3, src, dst);
}

void runasm_t::AND(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x21);
  modRM(0, src, DISP32);
  write32(dst);
}

void runasm_t::AND(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x23);
  modRM(0, dst, DISP32);
  write32(src);
}

// bitwise not instruction

void runasm_t::NOT(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 2, src);
}

// arithmetic negate instruction

void runasm_t::NEG(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 3, src);
}

void runasm_t::NOP() {
  write8(0x90);
}

void runasm_t::INT3() {
  write8(0xCC);
}

// jump instructions

rel8_t runasm_t::Jcc8(cc_t cc, label_t dst) {
  write8(0x70 | cc);
  write8(0);
  rel8_t rel = (rel8_t)(ptr - 1);
  if (rel) {
    setTarget(rel, dst);
  }
  return rel;
}

rel32_t runasm_t::Jcc32(cc_t cc, label_t dst) {
  write8(0x0F);
  write8(0x80 | cc);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  if (dst) {
    setTarget(rel, dst);
  }
  return rel;
}

rel8_t runasm_t::JMP8(label_t dst) {
  write8(0xEB);
  write8(0);
  rel8_t rel = (rel8_t)(ptr - 1);
  if (dst) {
    setTarget(rel, dst);
  }
  return rel;
}

rel32_t runasm_t::JMP32(label_t dst) {
  write8(0xE9);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  if (dst) {
    setTarget(rel, dst);
  }
  return rel;
}

void runasm_t::JMP(gp_reg32_t dst) {
  write8(0xFF);
  modRM(3, 4, dst);
}

// call subroutine instructions

void runasm_t::CALL(void *func) {
  assert(func);
  write8(0xE8);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  setTarget(rel, func);
}

rel32_t runasm_t::CALL(label_t dst) {
  write8(0xE8);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  if (dst) {
    setTarget(rel, dst);
  }
  return rel;
}

void runasm_t::CALL(gp_reg32_t dst) {
  write8(0xFF);
  modRM(3, 2, dst);
}

void runasm_t::CALL_32M(void *dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 2, DISP32);
  write32(dst);
}

// bit test instruction

void runasm_t::BT(gp_reg32_t dst, int32_t src) {
  write16(0xba0f);
  write8(0xe0 | dst);
  write8(src);
}

// compare instruction

void runasm_t::CMP(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x3D);
  } else {
    write8(0x81);
    modRM(3, 7, dst);
  }
  write32(src);
}

void runasm_t::CMP(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 7, DISP32);
  write32(dst);
  write32(src);
}

void runasm_t::CMP(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x39);
  modRM(3, src, dst);
}

void runasm_t::CMP(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x3B);
  modRM(0, dst, DISP32);
  write32(src);
}

// test instruction

void runasm_t::TEST(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0xA9);
  } else {
    write8(0xF7);
    modRM(3, 0, dst);
  }
  write32(src);
}

void runasm_t::TEST(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x85);
  modRM(3, src, dst);
}

// conditional byte set instructions

void runasm_t::SET(cc_t cc, gp_reg32_t dst) {
  write8(0x0F);
  write8(cc);
  write8(0xC0 | dst);
}

// convert byte dst word instruction

void runasm_t::CBW() {
  write16(0x9866);
}

// convert word dst doubleword instruction

void runasm_t::CWD() {
  write8(0x98);
}

// convert doubleword dst quadword instruction

void runasm_t::CDQ() {
  write8(0x99);
}

// stack push instructions

void runasm_t::PUSH(gp_reg32_t src) {
  write8(0x50 | src);
}

void runasm_t::PUSH(mem32_t src) {
  assert(src);
  write8(0xFF);
  modRM(0, 6, DISP32);
  write32(src);
}

void runasm_t::PUSH(uint32_t src) {
  write8(0x68);
  write32(src);
}

// stack pop instruction

void runasm_t::POP(gp_reg32_t src) {
  write8(0x58 | src);
}

// push general purpose registers

void runasm_t::PUSHA() {
  write8(0x60);
}

// pop general purpose registers

void runasm_t::POPA() {
  write8(0x61);
}

// return src subroutine instruction

void runasm_t::RET() {
  write8(0xC3);
}

void runasm_t::_mm_add_ps(xmm_reg_t dst, xmm_reg_t src) {
  write8(0x0f);
  write8(0x58);
  modRM(0x3, src, dst);
}

void runasm_t::_mm_sub_ps(xmm_reg_t dst, xmm_reg_t src) {
  write8(0x0f);
  write8(0x5c);
  modRM(0x3, src, dst);
}

void runasm_t::_mm_mul_ps(xmm_reg_t dst, xmm_reg_t src) {
  write8(0x0f);
  write8(0x59);
  modRM(0x3, src, dst);
}

} // namespace runasm
