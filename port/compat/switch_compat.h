// Force-included in every Switch translation unit. The wasm2c runtime assumes POSIX
// signals when not on Windows; libnx has plain setjmp/longjmp and no signal masks.
#pragma once
#include <setjmp.h>

#define sigsetjmp(buf, save) setjmp(buf)
#define siglongjmp(buf, val) longjmp(buf, val)
