#include <cstdio>
#include "crapjit.h"

namespace cj {

void *jit_factoral(crapjit_t & j) {

// prologue
    ir_t &fn = j.emit_label();
               j.emit_frame(0);
// if (val <= 1)
               j.emit_getl(2);
               j.emit_const(1);
               j.emit_leq();
    ir_t &l0 = j.emit_jz();
// return 1
               j.emit_const(1);
               j.emit_return(0);
// else
    l0.target( j.emit_label());
// return val * factoral(val - 1)
               j.emit_getl(2);
               j.emit_getl(2);
               j.emit_const(1);
               j.emit_sub();
               j.emit_call().target(fn);
               j.emit_sink(1);
               j.emit_mul();
               j.emit_return(0);

    return j.finish();
}

} // namespace cj

int main() {

    using namespace cj;

    typedef int32_t(*func_t)(int32_t v);

    crapjit_t j;
    // jit compile factoral
    func_t f = (func_t)jit_factoral(j);
    // excute factoral
    for (int i = 0; i < 10; ++i) {
        int32_t ret = f(i);
        printf("factoral(%d) -> %d\n", i, ret);
    }
    // result
    return 0;
}
