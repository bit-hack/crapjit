#include <cassert>
#include <cstring>

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

void runasm_t::write(const void* data, size_t count) {
  assert(size_t(end - ptr) >= count);
  memcpy(ptr, data, count);
  ptr += count;
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
  uint32_t jump = ((int8_t*)ptr - (int8_t *)j8) - 1;
  assert(jump <= 0x7f);
  *j8 = (uint8_t)jump;
}

void runasm_t::setTarget(rel32_t j32) {
  *j32 = ((int8_t*)ptr - (int8_t *)j32) - 4;
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
  ptr = (uint8_t *)(((uintptr_t)ptr + bytes) & ~(bytes - 1));
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

  const int8_t reg = 4;
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

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::MOV(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x89);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::MOV(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x89);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::MOV(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x8B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::MOV(gp_reg32_t dst, deref_t src) {
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
  postEmit();
}

void runasm_t::MOV(gp_reg32_t dst, const sib_t &src) {
  write8(0x8B);
  modRMSibSB(dst, src);
  postEmit();
}

void runasm_t::MOV(deref_t dst, gp_reg32_t src) {
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
  postEmit();
}

void runasm_t::MOV(const sib_t &dst, gp_reg32_t src) {
  write8(0x89);
  modRMSibSB(src, dst);
  postEmit();
}

void runasm_t::MOV(gp_reg32_t dst, uint32_t src) {
  write8(0xB8 | dst);
  write32(src);
  postEmit();
}

void runasm_t::MOV(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0xC7);
  modRM(0, 0, DISP32);
  write32(dst);
  write32(src);
  postEmit();
}

void runasm_t::MOV(mem16_t dst, gp_reg16_t src) {
  assert(dst);
  write8(0x66);
  write8(0x89);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::MOV(gp_reg16_t dst, mem16_t src) {
  assert(src);
  write8(0x66);
  write8(0x8B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::MOV(mem16_t dst, uint16_t src) {
  assert(dst);
  write8(0x66);
  write8(0xC7);
  modRM(0, 0, DISP32);
  write32(dst);
  write16(src);
  postEmit();
}

void runasm_t::MOV(mem8_t dst, gp_reg8_t src) {
  assert(dst);
  write8(0x88);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::MOV(gp_reg8_t dst, mem8_t src) {
  assert(src);
  write8(0x8A);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::MOV(mem8_t dst, uint8_t src) {
  assert(dst);
  assert(src);
  write8(0xC6);
  modRM(0, 0, DISP32);
  write32(dst);
  write8(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::MOVSX(gp_reg32_t dst, gp_reg8_t src) {
  write16(0xBE0F);
  modRM(3, dst, src);
  postEmit();
}

void runasm_t::MOVSX(gp_reg32_t dst, mem8_t src) {
  assert(src);
  write16(0xBE0F);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::MOVSX(gp_reg32_t dst, gp_reg16_t src) {
  write16(0xBF0F);
  modRM(3, dst, src);
  postEmit();
}

void runasm_t::MOVSX(gp_reg32_t dst, mem16_t src) {
  assert(src);
  write16(0xBF0F);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


void runasm_t::MOVZX(gp_reg32_t dst, gp_reg8_t src) {
  write16(0xB60F);
  modRM(3, dst, src);
  postEmit();
}

void runasm_t::MOVZX(gp_reg32_t dst, mem8_t src) {
  assert(src);
  write16(0xB60F);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::MOVZX(gp_reg32_t dst, gp_reg16_t src) {
  write16(0xB70F);
  modRM(3, dst, src);
  postEmit();
}

void runasm_t::MOVZX(gp_reg32_t dst, mem16_t src) {
  assert(src);
  write16(0xB70F);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CMOV(cc_t cc, gp_reg32_t dst, gp_reg32_t src) {
  write8(0x0F);
  write8(0x40 | cc);
  modRM(3, dst, src);
  postEmit();
}

void runasm_t::CMOV(cc_t cc, gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x0F);
  write8(0x40 | cc);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::ADD(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x05);
  } else {
    write8(0x81);
    modRM(3, 0, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::ADD(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 0, DISP32);
  write32(dst);
  write32(src);
  postEmit();
}

void runasm_t::ADD(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x01);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::ADD(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x01);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::ADD(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x03);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::ADD(deref_t dst, gp_reg32_t src) {
  write8(0x01);
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
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::ADC(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x15);
  } else {
    write8(0x81);
    modRM(3, 2, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::ADC(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x11);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::ADC(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x13);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::INC(gp_reg32_t dst) {
  write8(0x40 + dst);
  postEmit();
}

void runasm_t::INC(mem32_t dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 0, DISP32);
  write32(dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SUB(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x2D);
  } else {
    write8(0x81);
    modRM(3, 5, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::SUB(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x29);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::SUB(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x2B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::SUB(deref_t dst, gp_reg32_t src) {
  write8(0x29);
  assert(dst.is_reg());
  if (dst.reg == ESP) {
    // special case with SIB byte
    modRMSibSB(src, sib_t(1, dst.disp32, dst.reg));
  }
  else {
    if (dst.disp32) {
      modRM(2, src, dst.reg);
      write32(dst.disp32);
    }
    else {
      modRM(0, src, dst.reg);
    }
  }
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SBB(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x1D);
  } else {
    write8(0x81);
    modRM(3, 3, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::SBB(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x19);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::SBB(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x1B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::DEC(gp_reg32_t dst) {
  write8(0x48 + dst);
  postEmit();
}

void runasm_t::DEC(mem32_t dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 1, DISP32);
  write32(dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::MUL(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 4, src);
  postEmit();
}

void runasm_t::MUL(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 4, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::IMUL(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 5, src);
  postEmit();
}

void runasm_t::IMUL(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 5, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::IMUL(gp_reg32_t dst, gp_reg32_t src) {
  write16(0xAF0F);
  modRM(3, dst, src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::DIV(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 6, src);
  postEmit();
}

void runasm_t::DIV(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 6, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::IDIV(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 7, src);
  postEmit();
}

void runasm_t::IDIV(mem32_t src) {
  assert(src);
  write8(0xF7);
  modRM(0, 7, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::RCR(int32_t dst, int32_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xd8 | dst);
  } else {
    write8(0xc1);
    write8(0xd8 | dst);
    write8(src);
  }
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SHL(gp_reg32_t dst, uint8_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xe0 | dst);
    return;
  }
  write8(0xC1);
  modRM(3, 4, dst);
  write8(src);
  postEmit();
}

void runasm_t::SHL(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 4, dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SHR(gp_reg32_t dst, uint8_t src) {
  if (src == 1) {
    write8(0xd1);
    write8(0xe8 | dst);
    return;
  }
  write8(0xC1);
  modRM(3, 5, dst);
  write8(src);
  postEmit();
}

void runasm_t::SHR(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 5, dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SAR(gp_reg32_t dst, uint8_t src) {
  write8(0xC1);
  modRM(3, 7, dst);
  write8(src);
  postEmit();
}

void runasm_t::SAR(gp_reg32_t dst) {
  write8(0xD3);
  modRM(3, 7, dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::OR(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x0D);
  } else {
    write8(0x81);
    modRM(3, 1, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::OR(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 1, DISP32);
  write32(dst);
  write32(src);
  postEmit();
}

void runasm_t::OR(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x09);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::OR(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x09);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::OR(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x0B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::XOR(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x35);
  } else {
    write8(0x81);
    modRM(3, 6, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::XOR(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 6, DISP32);
  write32(dst);
  write32(src);
  postEmit();
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
  postEmit();
}

void runasm_t::XOR(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x33);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::AND(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x25);
  } else {
    write8(0x81);
    modRM(3, 0x4, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::AND(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 0x4, DISP32);
  write32(dst);
  write32(src);
  postEmit();
}

void runasm_t::AND(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x21);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::AND(mem32_t dst, gp_reg32_t src) {
  assert(dst);
  write8(0x21);
  modRM(0, src, DISP32);
  write32(dst);
  postEmit();
}

void runasm_t::AND(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x23);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::NOT(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 2, src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::NEG(gp_reg32_t src) {
  write8(0xF7);
  modRM(3, 3, src);
  postEmit();
}

void runasm_t::NOP() {
  write8(0x90);
  postEmit();
}

void runasm_t::INT3() {
  write8(0xCC);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

rel8_t runasm_t::Jcc8(cc_t cc, label_t dst) {
  write8(0x70 | cc);
  write8(0);
  rel8_t rel = (rel8_t)(ptr - 1);
  if (rel) {
    setTarget(rel, dst);
  }
  peepCeil = ptr;
  postEmit();
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
  peepCeil = ptr;
  postEmit();
  return rel;
}

rel8_t runasm_t::JMP8(label_t dst) {
  write8(0xEB);
  write8(0);
  rel8_t rel = (rel8_t)(ptr - 1);
  if (dst) {
    setTarget(rel, dst);
  }
  peepCeil = ptr;
  postEmit();
  return rel;
}

rel32_t runasm_t::JMP32(label_t dst) {
  write8(0xE9);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  if (dst) {
    setTarget(rel, dst);
  }
  peepCeil = ptr;
  postEmit();
  return rel;
}

void runasm_t::JMP(gp_reg32_t dst) {
  write8(0xFF);
  modRM(3, 4, dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CALL(void *func) {
  assert(func);
  write8(0xE8);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  setTarget(rel, func);
  postEmit();
}

rel32_t runasm_t::CALL(label_t dst) {
  write8(0xE8);
  write32(0u);
  rel32_t rel = (rel32_t)(ptr - 4);
  if (dst) {
    setTarget(rel, dst);
  }
  peepCeil = ptr;
  postEmit();
  return rel;
}

void runasm_t::CALL(gp_reg32_t dst) {
  write8(0xFF);
  modRM(3, 2, dst);
  postEmit();
}

void runasm_t::CALL_32M(void *dst) {
  assert(dst);
  write8(0xFF);
  modRM(0, 2, DISP32);
  write32(dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::BT(gp_reg32_t dst, int32_t src) {
  write16(0xba0f);
  write8(0xe0 | dst);
  write8(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CMP(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0x3D);
  } else {
    write8(0x81);
    modRM(3, 7, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::CMP(mem32_t dst, uint32_t src) {
  assert(dst);
  write8(0x81);
  modRM(0, 7, DISP32);
  write32(dst);
  write32(src);
  postEmit();
}

void runasm_t::CMP(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x39);
  modRM(3, src, dst);
  postEmit();
}

void runasm_t::CMP(gp_reg32_t dst, mem32_t src) {
  assert(src);
  write8(0x3B);
  modRM(0, dst, DISP32);
  write32(src);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::TEST(gp_reg32_t dst, uint32_t src) {
  if (dst == EAX) {
    write8(0xA9);
  } else {
    write8(0xF7);
    modRM(3, 0, dst);
  }
  write32(src);
  postEmit();
}

void runasm_t::TEST(gp_reg32_t dst, gp_reg32_t src) {
  write8(0x85);
  modRM(3, src, dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::SET(cc_t cc, gp_reg32_t dst) {
  write8(0x0F);
  write8(0x90 | cc);
  write8(0xC0 | dst);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CBW() {
  write16(0x9866);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CWD() {
  write8(0x98);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::CDQ() {
  write8(0x99);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::PUSH(gp_reg32_t src) {
  write8(0x50 | src);
  postEmit();
}

void runasm_t::PUSH(mem32_t src) {
  assert(src);
  write8(0xFF);
  modRM(0, 6, DISP32);
  write32(src);
  postEmit();
}

void runasm_t::PUSH(uint32_t src) {
  write8(0x68);
  write32(src);
  postEmit();
}

void runasm_t::PUSH(const sib_t& sib) {
  // ff b0 45 23 01 00       push   DWORD PTR[eax + 0x12345]
  write8(0xff);
  modRMSibSB(0b110, sib);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::POP(gp_reg32_t src) {
  write8(0x58 | src);
  postEmit();
}

void runasm_t::POP(const sib_t& sib) {
  // 8f 83 45 23 01 00       pop    DWORD PTR [ebx+0x12345]
  write8(0x8f);
  modRMSibSB(0b000, sib);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::PUSHA() {
  write8(0x60);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::POPA() {
  write8(0x61);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::RET() {
  write8(0xC3);
  postEmit();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void runasm_t::postEmit() {

  static const uint8_t PUSH_EAX = 0x50;
  static const uint8_t POP_EAX  = 0x58;

  do {

    const size_t space = ptr - peepCeil;

    // [ push eax | pop eax ] => []
    if (space >= 2) {
      if (prior8(2) == PUSH_EAX &&
          prior8(1) == POP_EAX) {
        ptr -= 2;
        continue;
      }
    }

    // [ mov eax | 0xaabbccdd | push eax ] => [ push 0xaabbccdd ]
    if (space >= 6) {
      if (prior8(6) == 0xB8 &&
          prior8(1) == PUSH_EAX) {
        const uint32_t imm = prior32(5);
        ptr -= 6;
        PUSH(imm);
        continue;
      }
    }

    // [ push 0xaabbccdd | pop eax ] => [ mov eax 0xaabbccdd ]
    if (space >= 6) {
      if (prior8(6) == 0x68 && prior8(1) == POP_EAX) {
        const uint32_t imm = prior32(5);
        ptr -= 6;
        MOV(EAX, imm);
        continue;
      }
    }

    // [ mov eax 0xaabbccdd | pop edx | cmp edx eax ] => [ pop edx | cmp edx, 0xaabbccdd ]
    if (space >= 8) {
      if (prior8(8) == 0xb8 && prior8(3) == 0x5a && prior8(2) == 0x39 && prior8(1) == 0xc2) {
        const uint32_t imm = prior32(7);
        ptr -= 8;
        POP(EDX);
        CMP(EDX, imm);
        continue;
      }
    }

    // [ push eax | pop edx ] => [ mov edx, eax ]
    // 50       push   eax
    // 5a       pop    edx
    if (space >= 2) {
      if (prior8(2) == 0x50 && prior8(1) == 0x5a) {
        ptr -= 2;
        MOV(EDX, EAX);
        continue;
      }
    }

    // [ mov edx eax | cmp edx 0xaabbccdd ] => [ cmp eax 0xaabbccdd ]
    // 89 c2                   mov    edx, eax
    // 81 fa 01 00 00 00       cmp    edx, 0x1
    if (space >= 8) {
      if (prior8(8) == 0x89 && prior8(7) == 0xc2 && prior8(6) == 0x81 && prior8(5) == 0xfa) {
        const uint32_t imm = prior32(4);
        ptr -= 8;
        CMP(EAX, imm);
        continue;
      }
    }

    // [ mov eax 0xaabbccdd | sub [esp+0] eax ] => [ sub [esp+0] 0xaabbccdd ]
    // b8 01 00 00 00          mov    eax, 0x1
    // 29 44 24 00             sub    DWORD PTR[esp + 0x0], eax
    if (space >= 9) {
      if (prior8(9) == 0xb8 && prior8(4) == 0x29 && prior8(3) == 0x44 && prior8(2) == 0x24 && prior8(1) == 0x00) {
        const uint32_t imm = prior32(8);
        ptr -= 9;
        write("\x81\x2c\x24", 3);
        write32(imm);
        continue;
      }
    }

    break;

  } while (true);
}

} // namespace runasm
