// PC test harness: runs the recompiled game headless and reports how far boot gets.
//   tmnt2_pc.exe <asset dir> <data dir> [frames] [-v]
#include <setjmp.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include "rt.h"
#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"

w2c_tmnt2 g_inst;
struct w2c_a g_env;

extern jmp_buf rt_main_exit;
extern uint32_t rt_main_exit_depth;
extern unsigned long rt_frames;
int rt_run_frame(void);

static const char* g_asset_dir;
static const char* g_data_dir;

const char* rt_asset_dir(void) { return g_asset_dir; }
const char* rt_data_dir(void) { return g_data_dir; }

double rt_now_ms(void) {
  static LARGE_INTEGER freq, start;
  LARGE_INTEGER now;
  if (!freq.QuadPart) {
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
  }
  QueryPerformanceCounter(&now);
  return (double)(now.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart;
}

double rt_epoch_ms(void) { return (double)time(NULL) * 1000.0; }
void rt_sleep_ms(double ms) { Sleep((DWORD)ms); }
int rt_platform_frame(void) { return 1; }

// argv for the wasm main(): {"./this.program", "-full", NULL} built on the wasm stack
static u32 rt_push_string(const char* s) {
  u32 len = (u32)strlen(s) + 1;
  u32 p = rt_stackAlloc(len);
  memcpy(RT_PTR(p), s, len);
  return p;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <asset dir> <data dir> [frames] [-v]\n", argv[0]);
    return 2;
  }
  g_asset_dir = argv[1];
  g_data_dir = argv[2];
  unsigned long max_frames = argc > 3 ? strtoul(argv[3], NULL, 10) : 300;
  rt_verbose = argc > 4 && strcmp(argv[4], "-v") == 0;
  setvbuf(stderr, NULL, _IONBF, 0);

  wasm_rt_init();
  wasm2c_tmnt2_instantiate(&g_inst, &g_env);
  RT_LOG("[boot] instantiated, memory %u bytes\n", (unsigned)RT_MEMORY->size);

  wasm_rt_trap_t trap = wasm_rt_impl_try();
  if (trap != WASM_RT_TRAP_NONE) {
    RT_LOG("[trap] %s (after %lu buffer swaps)\n", wasm_rt_strerror(trap), rt_frames);
    return 1;
  }

  rt_wasm_call_ctors();
  RT_LOG("[boot] ctors done\n");

  const char* args[] = {"./this.program", "-full"};
  u32 wasm_argv = rt_stackAlloc(4 * 3);
  for (int i = 0; i < 2; i++) RT_U32(wasm_argv + 4 * i) = rt_push_string(args[i]);
  RT_U32(wasm_argv + 8) = 0;

#if WASM_RT_STACK_DEPTH_COUNT
  rt_main_exit_depth = wasm_rt_call_stack_depth;
#endif
  if (setjmp(rt_main_exit) == 0) {
    u32 ret = rt_main(2, wasm_argv);
    RT_LOG("[boot] main returned %u\n", ret);
  } else {
    RT_LOG("[boot] main handed over to the main loop\n");
  }

  double start = rt_now_ms();
  for (unsigned long i = 0; i < max_frames; i++) {
    rt_run_frame();
    if (i % 60 == 0)
      RT_LOG("[loop] iteration %lu, swaps %lu, %.1fs\n", i, rt_frames, (rt_now_ms() - start) / 1000.0);
    rt_sleep_ms(1);
  }
  RT_LOG("[done] %lu iterations, %lu buffer swaps\n", max_frames, rt_frames);
  wasm2c_tmnt2_free(&g_inst);
  wasm_rt_free();
  return 0;
}
