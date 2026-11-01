// Host runtime shared by the PC harness and the Switch build.
// The wasm2c module imports everything from module "a"; this header gives the
// implementations access to the instance, its linear memory and its exports.
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tmnt2.h"

struct w2c_a {
  int unused;
};

extern w2c_tmnt2 g_inst;
extern struct w2c_a g_env;

// Linear memory can be reallocated on growth, so always go through g_inst.
#define RT_MEMORY (w2c_tmnt2_pe(&g_inst))
#define RT_PTR(addr) ((void*)(RT_MEMORY->data + (uint32_t)(addr)))
#define RT_U32(addr) (*(uint32_t*)RT_PTR(addr))
#define RT_I32(addr) (*(int32_t*)RT_PTR(addr))
#define RT_F32(addr) (*(float*)RT_PTR(addr))
#define RT_STR(addr) ((addr) ? (const char*)RT_PTR(addr) : "")

// Exports, named after the emscripten symbols they were minified from.
#define rt_wasm_call_ctors()            w2c_tmnt2_qe(&g_inst)
#define rt_malloc(n)                    w2c_tmnt2_re(&g_inst, (n))
#define rt_free(p)                      w2c_tmnt2_se(&g_inst, (p))
#define rt_onWorkerFSReady()            w2c_tmnt2_ve(&g_inst)
#define rt_onWorkerFSMountReady()       w2c_tmnt2_we(&g_inst)
#define rt_onWorkerFSDataReady(b,s,o,p) w2c_tmnt2_xe(&g_inst, (b), (s), (o), (p))
#define rt_onMainModuleReady()          w2c_tmnt2_Ae(&g_inst)
#define rt_main(argc, argv)             w2c_tmnt2_Be(&g_inst, (argc), (argv))
#define rt_emscripten_timeout(id, now)  w2c_tmnt2_Te(&g_inst, (id), (now))
#define rt_setThrew(t, v)               w2c_tmnt2_Ue(&g_inst, (t), (v))
#define rt_stackRestore(sp)             w2c_tmnt2_Ve(&g_inst, (sp))
#define rt_stackAlloc(n)                w2c_tmnt2_We(&g_inst, (n))
#define rt_stackSave()                  w2c_tmnt2_Xe(&g_inst)

// Logging: every stub reports its first few calls so a run shows which imports matter.
extern int rt_verbose;
#define RT_LOG(...) fprintf(stderr, __VA_ARGS__)
#define RT_STUB_LOG(name, fmt, ...)                                   \
  do {                                                                \
    static unsigned rt_calls_;                                        \
    if (rt_verbose || rt_calls_ < 3)                                  \
      RT_LOG("[stub] %s" fmt "\n", name, ##__VA_ARGS__);              \
    if (++rt_calls_ == 3 && !rt_verbose)                              \
      RT_LOG("[stub] %s ... (further calls muted)\n", name);          \
  } while (0)

// Platform layer (pc_platform.c / switch_platform.c).
double rt_now_ms(void);              // monotonic milliseconds
double rt_epoch_ms(void);            // wall clock milliseconds since 1970
void rt_sleep_ms(double ms);
const char* rt_asset_dir(void);      // folder holding TMNT.DAT etc.
const char* rt_data_dir(void);       // folder holding the preloaded /Common files and saves
int rt_platform_frame(void);         // pump events, swap buffers; returns 0 to quit

// Core runtime (rt_core.c).
void rt_pump_async(void);            // deliver queued file callbacks
int rt_run_frame(void);              // one iteration of the game's main loop

// Input (rt_input.c). Buttons use W3C "standard gamepad" order:
// 0 A(bottom) 1 B(right) 2 X(left) 3 Y(top) 4 LB 5 RB 6 LT 7 RT 8 Back 9 Start
// 10 LS 11 RS 12 Up 13 Down 14 Left 15 Right 16 Home 17 extra; axes: LX LY RX RY in -1..1.
// SDL3 turns 6/7 into axes and 12-15 into a hat, so the game's joystick buttons 0..11 are
// W3C 0 1 2 3 4 5 8 9 10 11 16 17 (index 17 exists only so the game gets its 12th button).
#define RT_PAD_BUTTONS 18
#define RT_PAD_AXES 4
typedef struct {
  uint8_t buttons[RT_PAD_BUTTONS];
  float axes[RT_PAD_AXES];
} rt_pad;
void rt_input_update(const rt_pad* pad);

// rt_fs.c: create a game directory (IDBFS mount point) on the host.
void rt_fs_mkdir(const char* wasm_path);
