#if defined(_MSC_VER)
#include <Windows.h>
#include "system.h"

extern void * code_alloc(uint32_t size) {
    return VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

extern void code_free(void *ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}
#else
#include <sys/mman.h>
#include "system.h"

extern void * code_alloc(uint32_t size) {
    const int prot = PROT_EXEC | PROT_READ | PROT_WRITE;
    const int flag = MAP_ANONYMOUS | MAP_PRIVATE;
    void * mem = mmap(nullptr, size, prot, flag, -1, 0);
    return mem;
}

extern void code_free(void *ptr) {
    munmap(ptr, 0);
}
#endif
