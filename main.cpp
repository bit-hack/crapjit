#include <cstdio>
#include "crapjit.h"

label_t jit_factoral(crapjit_t & j) {

    reloc_t l0;
    label_t fn;
// prologue
    fn = j.emit_label();
         j.emit_frame(0);
// if (val <= 1)
         j.emit_getl(2);
         j.emit_const(1);
         j.emit_leq();
    l0 = j.emit_jz();
// return 1
         j.emit_const(1);
         j.emit_return(0);
// else
    l0.set(j.emit_label());
// return val * factoral(val - 1)
         j.emit_getl(2);
         j.emit_getl(2);
         j.emit_const(1);
         j.emit_sub();
         j.emit_call().set(fn);
         j.emit_sink(1);
         j.emit_mul();
         j.emit_return(0);
    return fn;
}

int main() {

    crapjit_t j;
    // jit compile factoral
    label_t factoral = jit_factoral(j);
    // cast to function pointer
    typedef int32_t(*func_t)(int32_t v);
    func_t f = (func_t)factoral;
    // excute factoral
    for (int i = 0; i < 10; ++i) {
        int32_t ret = f(i);
        printf("factoral(%d) -> %d\n", i, ret);
    }
    // result
	return 0;
}
