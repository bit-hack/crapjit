#pragma once
#include <stdint.h>

extern void * code_alloc(uint32_t size);
extern void   code_free (void *);
