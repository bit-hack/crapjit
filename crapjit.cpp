#include <cstring>
#include "crapjit.h"
#include "chunks.h"
#include "system.h"

#if defined(_MSC_VER)
#define assert(X) { if (!(X)) __debugbreak(); }
#else
#include <cassert>
#endif

extern jit_chunk_t chunk_table[];

template <typename type_t>
void insert(uint8_t * & ptr, const type_t val) {
    const uint32_t size = sizeof(type_t);
    memcpy(ptr, &val, size);
    ptr += size;
}

void insert(uint8_t * & ptr, const void * data, const uint32_t size) {
    memcpy(ptr, data, size);
    ptr += size;
}

crapjit_t::crapjit_t()
    : did_label_(true)
{
    data_ = head_ = (uint8_t*)code_alloc(1024 * 512);
}

crapjit_t::~crapjit_t() {
    code_free(data_);
}

void reloc_t::set(label_t pos) {
    assert(base_[2] == 0xbb);
    if (type_ == RELOC_ABS) {
        insert<label_t>(base_, pos);
    }
    if (type_ == RELOC_REL) {
        int32_t org = int32_t(base_) + 4;
        int32_t dst = int32_t(pos);
        insert<int32_t>(base_, dst - org);
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

reloc_t crapjit_t::emit_inst(uint8_t * & ptr, jit_op op) {

    const bool optimize = true;

    const jit_chunk_t & chunk = chunk_table[op];

    reloc_t reloc;

    const uint8_t PUSH_EAX = '\x50';
    const uint8_t POP_EAX  = '\x58';

    uint8_t * base = ptr;

    // as an optimization, we can merge chunks ending with PUSH_EAX
    // and starting with POP_EAX
    if (optimize && 
        !did_label_ && 
        chunk.data_[0] == POP_EAX && 
        ptr[-1] == PUSH_EAX) {
        ptr  -= 1;          // slide back to cover push eax
        base  = ptr - 1;    // -1 to account for dropping pop eax later
        insert(ptr, chunk.data_+1, chunk.size_-1);
    }
    else {
        insert(ptr, chunk.data_, chunk.size_);
    }

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

    did_label_ = false;
    return reloc;
}

void crapjit_t::emit_const(int32_t val) {
    emit_inst(head_, ins_const).imm_i32(val);
}

void crapjit_t::emit_drop() {
    emit_inst(head_, ins_drop);
}

void crapjit_t::emit_dup() {
    emit_inst(head_, ins_dup);
}

void crapjit_t::emit_sink(uint32_t val) {
    emit_inst(head_, ins_sink).imm_u32(val*4);
}

void crapjit_t::emit_getl(int32_t offs) {
    emit_inst(head_, ins_getl).imm_i32(offs*4);
}

void crapjit_t::emit_setl(int32_t offs) {
    emit_inst(head_, ins_setl).imm_i32(offs*4);
}

void crapjit_t::emit_frame(uint32_t val) {
    emit_inst(head_, ins_frame).imm_u32(val*4);
}

void crapjit_t::emit_return(uint32_t val) {
    emit_inst(head_, ins_ret).imm_u32(val*4);
}

reloc_t crapjit_t::emit_call() {
    return emit_inst(head_, ins_call);
}

reloc_t crapjit_t::emit_jz() {
    return emit_inst(head_, ins_jz);
}

reloc_t crapjit_t::emit_jnz() {
    return emit_inst(head_, ins_jnz);
}

reloc_t crapjit_t::emit_jmp() {
    return emit_inst(head_, ins_jmp);
}

label_t crapjit_t::emit_label() {
    did_label_ = true;
    return head_;
}

void crapjit_t::emit_add() {
    emit_inst(head_, ins_add);
}

void crapjit_t::emit_sub() {
    emit_inst(head_, ins_sub);
}

void crapjit_t::emit_mul() {
    emit_inst(head_, ins_mul);
}

void crapjit_t::emit_div() {
    emit_inst(head_, ins_div);
}

void crapjit_t::emit_and() {
    emit_inst(head_, ins_and);
}

void crapjit_t::emit_or() {
    emit_inst(head_, ins_or);
}

void crapjit_t::emit_lt() {
    emit_inst(head_, ins_lt);
}

void crapjit_t::emit_leq() {
    emit_inst(head_, ins_lte);
}

void crapjit_t::emit_gt() {
    emit_inst(head_, ins_gt);
}

void crapjit_t::emit_geq() {
    emit_inst(head_, ins_gte);
}

void crapjit_t::emit_eq() {
    emit_inst(head_, ins_eq);
}

void crapjit_t::emit_neq() {
    emit_inst(head_, ins_neq);
}
