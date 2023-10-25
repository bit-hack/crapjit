#include <cstdio>
#include "libcj/crapjit.h"

static void banner(const char* name) {
  printf("%20s : ", name);
}

static uint32_t hash(uint32_t input) {
  uint32_t state = input * 747796405u + 2891336453u;
  uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_return() {
  banner(__func__);

  using namespace cj;
  typedef void (*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);      // return needs a frame to be setup
  j.emit_const(0);      // return needs to pop a constant
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  f();

  return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_const() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t (*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(0xcafebabe);
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return ret == 0xcafebabe;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_add() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(11);
  j.emit_const(1234);
  j.emit_add();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return ret == 1245;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_sub() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(1234);
  j.emit_const(11);
  j.emit_sub();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return ret == 1223;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_and() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(0x00ff00ff);
  j.emit_const(0x003f0080);
  j.emit_and();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return ret == 0x003f0080;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_or() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(0x00f0007f);
  j.emit_const(0x00f03480);
  j.emit_or();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return ret == 0x00f034ff;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_mul() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(12);
  j.emit_const(5);
  j.emit_mul();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return 60 == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_jmp() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  // note: test backward branches too

  crapjit_t j;
            j.emit_frame(0);
  ir_t &L = j.emit_jmp();           // taken ----.
            j.emit_const(0xdead);   // fail      |
            j.emit_return(0);       // XXXXXXXXX |
  L.target( j.emit_label() );       // <---------'
            j.emit_const(0xbeef);   // pass
            j.emit_return(0);       // XXXXXXXXX

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return 0xbeef == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_jz() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  // note: test taken / not-taken
  // note: test backward branches

  crapjit_t j;
            j.emit_frame(0);
            j.emit_const(0);
  ir_t &L = j.emit_jz();            // taken ----.
  ir_t &T = j.emit_label();         // <---------|-.
            j.emit_const(0xdead);   //           | |
            j.emit_return(0);       // XXXXXXXXXX|X|XXX FAIL
  L.target( j.emit_label() );       // <---------' |
            j.emit_const(1);        //             |
  ir_t &M = j.emit_jz();            // not taken --'
            j.emit_const(0xbeef);   //
            j.emit_return(0);       // XXXXXXXXXXXXXXXX PASS
  M.target(T);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return 0xbeef == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_jnz() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(void);

  // note: test taken / not-taken
  // note: test backward branches

  crapjit_t j;
            j.emit_frame(0);
            j.emit_const(1);
  ir_t &L = j.emit_jnz();           // taken ----.
  ir_t &T = j.emit_label();         // <---------|-.
            j.emit_const(0xdead);   //           | |
            j.emit_return(0);       // XXXXXXXXXX|X|XX FAIL
  L.target( j.emit_label() );       // <---------' |
            j.emit_const(0);        //             |
  ir_t &M = j.emit_jnz();           // not taken --'
            j.emit_const(0xbeef);   //
            j.emit_return(0);       // XXXXXXXXXXXXXXX PASS
  M.target(T);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f();

  return 0xbeef == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_args_1() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(uint32_t);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_getl(2);
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();
  const uint32_t ret = f(0xbeef);

  return 0xbeef == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_args_2() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)(uint32_t, uint32_t);

  crapjit_t j;
  j.emit_frame(0);
  j.emit_getl(2);   // lhs
  j.emit_getl(3);   // rhs
  j.emit_sub();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();

  for (uint32_t i = 0; i < 100; ++i) {
    const uint32_t lhs = hash(i);
    const uint32_t rhs = hash(1 ^ 0xbeef);
    const uint32_t ret = f(lhs, rhs);
    if (ret != (lhs - rhs)) {
      return false;
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_frame() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)();

  crapjit_t j;
  j.emit_frame(1);
  j.emit_const(0xc0ffee);
  j.emit_return(1);

  jitfunc_t f = (jitfunc_t)j.finish();

  const uint32_t ret = f();
  return 0xc0ffee == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_drop() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)();

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(0xc0ffee);
  j.emit_const(0xbad1bad1);
  j.emit_drop(1);
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();

  const uint32_t ret = f();
  return 0xc0ffee == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_dup() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)();

  crapjit_t j;
  j.emit_frame(0);
  j.emit_const(13);
  j.emit_dup();
  j.emit_add();
  j.emit_return(0);

  jitfunc_t f = (jitfunc_t)j.finish();

  const uint32_t ret = f();
  return 26 == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// test loops
// have an increment loop (for i = 0 to 10) sort of thing

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool test_locals() {
  banner(__func__);

  using namespace cj;
  typedef uint32_t(*jitfunc_t)();

  // todo: verify this properly!

  crapjit_t j;
  j.emit_frame(1);
  j.emit_const(0xc0ffee);
  j.emit_setl(-1);
  j.emit_getl(-1);
  j.emit_return(1);

  jitfunc_t f = (jitfunc_t)j.finish();

  const uint32_t ret = f();
  return 0xc0ffee == ret;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void *jit_factoral(cj::crapjit_t & j) {
  using namespace cj;

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
    l0.target( j.emit_label() );
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

static bool test_factoral() {
  banner(__func__);

  static const uint32_t expect[] = {
    1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880,
  };

  // compile factoral
  cj::crapjit_t j;
  typedef int32_t(*func_t)(int32_t v);
  func_t f = (func_t)jit_factoral(j);

  // excute factoral over a range of inputs
  for (int i = 0; i < 10; ++i) {
    const int32_t ret = f(i);
    if (ret != expect[i]) {
      return false;  // fail
    }
  }

  return true;  // pass
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
typedef bool (*test_t)(void);

static const test_t tests[] = {
  test_return,
  test_const,
  test_add,
  test_sub,
  test_and,
  test_or,
  test_mul,
  test_jmp,
  test_jz,
  test_jnz,
  test_drop,
  test_dup,
  test_args_1,
  test_args_2,
  test_frame,
  test_locals,
  test_factoral,
  nullptr
};

int main() {

    for (uint32_t i = 0; tests[i]; ++i) {
        fflush(stdout);
        const bool res = tests[i]();
        printf("%s\n", res ? "pass" : "fail");
    }

    return 0;
}
