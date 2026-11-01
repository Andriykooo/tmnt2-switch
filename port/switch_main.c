// Nintendo Switch entry point: EGL/GLES on the default window, Joy-Con input, and the
// recompiled game running on a thread with a large stack.
//
// SD card layout:
//   sdmc:/switch/tmnt2/tmnt2.nro
//   sdmc:/switch/tmnt2/TMNT.DAT, TMNTE.DAT, TMNT2V0x.DAT, WaveData.bin, SeData.bin, TrSeData.bin
//   sdmc:/switch/tmnt2/Common/Fonts/MainFont.met, Common/Touch/Touch.lpac
//   sdmc:/switch/tmnt2/log.txt   (written on every run)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <switch.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "rt.h"
#include "wasm-rt-exceptions.h"
#include "wasm-rt-impl.h"

#define GAME_DIR "sdmc:/switch/tmnt2"
#define GAME_STACK_SIZE (16 * 1024 * 1024)

w2c_tmnt2 g_inst;
struct w2c_a g_env;

extern jmp_buf rt_main_exit;
extern uint32_t rt_main_exit_depth;
extern unsigned long rt_frames;

static EGLDisplay g_display;
static EGLSurface g_surface;
static EGLContext g_context;
static PadState g_padstate;
static int g_quit = 0;

const char* rt_asset_dir(void) { return GAME_DIR; }
const char* rt_data_dir(void) { return GAME_DIR; }

double rt_now_ms(void) { return (double)armTicksToNs(armGetSystemTick()) / 1e6; }
double rt_epoch_ms(void) { return (double)time(NULL) * 1000.0; }
void rt_sleep_ms(double ms) { svcSleepThread((s64)(ms * 1e6)); }

/* ---------------- display ---------------- */

static int egl_init(void) {
  NWindow* win = nwindowGetDefault();
  nwindowSetDimensions(win, 1280, 720);

  g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (!g_display || !eglInitialize(g_display, NULL, NULL)) return 0;
  eglBindAPI(EGL_OPENGL_ES_API);

  static const EGLint config_attrs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
      EGL_NONE};
  EGLConfig config;
  EGLint count = 0;
  if (!eglChooseConfig(g_display, config_attrs, &config, 1, &count) || count == 0) return 0;

  g_surface = eglCreateWindowSurface(g_display, config, win, NULL);
  if (!g_surface) return 0;

  static const EGLint ctx_attrs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
  g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, ctx_attrs);
  if (!g_context) return 0;

  eglMakeCurrent(g_display, g_surface, g_surface, g_context);
  eglSwapInterval(g_display, 1);
  return 1;
}

static void egl_exit(void) {
  if (!g_display) return;
  eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (g_context) eglDestroyContext(g_display, g_context);
  if (g_surface) eglDestroySurface(g_display, g_surface);
  eglTerminate(g_display);
  g_display = NULL;
}

/* ---------------- input ---------------- */

static float stick(s32 v) { return v / 32767.0f; }

static void read_pad(void) {
  padUpdate(&g_padstate);
  u64 held = padGetButtons(&g_padstate);
  HidAnalogStickState l = padGetStickPos(&g_padstate, 0);
  HidAnalogStickState r = padGetStickPos(&g_padstate, 1);

  // Slots are chosen by what the game does with them (see rt.h for the W3C -> game chain):
  //   A attack A / OK     B jump / cancel    X attack B         Y dash
  //   L shuriken          R, ZR guard        ZL switch turtle   (R + X = grab, then X = throw)
  //   + pause (sent as Escape by rt_input.c) - select   L3 gauge   R3 camera
  rt_pad pad = {0};
  pad.buttons[0] = !!(held & HidNpadButton_A);                        // RDOWN  attack A
  pad.buttons[1] = !!(held & HidNpadButton_X);                        // RLEFT  attack B
  pad.buttons[2] = !!(held & HidNpadButton_B);                        // RRIGHT jump
  pad.buttons[3] = !!(held & HidNpadButton_L);                        // RUP    shuriken
  pad.buttons[4] = !!(held & HidNpadButton_Y);                        // L1     dash
  pad.buttons[5] = !!(held & HidNpadButton_ZL);                       // L2     switch turtle
  pad.buttons[8] = !!(held & (HidNpadButton_R | HidNpadButton_ZR));   // R1     guard
  pad.buttons[9] = !!(held & HidNpadButton_Plus);                     // R2     unused -> Escape
  pad.buttons[10] = !!(held & HidNpadButton_Minus);                   // SELECT
  pad.buttons[16] = !!(held & HidNpadButton_StickL);                  // L3     gauge
  pad.buttons[17] = !!(held & HidNpadButton_StickR);                  // R3     camera
  pad.buttons[12] = !!(held & HidNpadButton_Up);
  pad.buttons[13] = !!(held & HidNpadButton_Down);
  pad.buttons[14] = !!(held & HidNpadButton_Left);
  pad.buttons[15] = !!(held & HidNpadButton_Right);
  pad.axes[0] = stick(l.x);
  pad.axes[1] = -stick(l.y);  // W3C: down is positive
  pad.axes[2] = stick(r.x);
  pad.axes[3] = -stick(r.y);
  rt_input_update(&pad);

  // Hold + and - together to quit.
  if ((held & HidNpadButton_Plus) && (held & HidNpadButton_Minus)) g_quit = 1;
}

int rt_platform_frame(void) {
  eglSwapBuffers(g_display, g_surface);
  read_pad();
  return appletMainLoop() && !g_quit;
}

/* ---------------- game thread ---------------- */

static int g_exit_code = 0;

static u32 push_string(const char* s) {
  u32 len = (u32)strlen(s) + 1;
  u32 p = rt_stackAlloc(len);
  memcpy(RT_PTR(p), s, len);
  return p;
}

static void game_thread(void* arg) {
  if (!egl_init()) {
    RT_LOG("[boot] EGL init failed\n");
    g_exit_code = 1;
    return;
  }
  RT_LOG("[boot] GL: %s / %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

  wasm_rt_init();
  wasm2c_tmnt2_instantiate(&g_inst, &g_env);

  wasm_rt_trap_t trap = wasm_rt_impl_try();
  if (trap != WASM_RT_TRAP_NONE) {
    RT_LOG("[trap] %s after %lu frames\n", wasm_rt_strerror(trap), rt_frames);
    g_exit_code = 2;
    egl_exit();
    return;
  }

  rt_wasm_call_ctors();
  u32 argv = rt_stackAlloc(4 * 3);
  RT_U32(argv) = push_string("./this.program");
  RT_U32(argv + 4) = push_string("-full");
  RT_U32(argv + 8) = 0;

#if WASM_RT_STACK_DEPTH_COUNT
  rt_main_exit_depth = wasm_rt_call_stack_depth;
#endif
  if (setjmp(rt_main_exit) == 0) {
    u32 ret = rt_main(2, argv);
    RT_LOG("[boot] main returned %u\n", ret);
  } else {
    RT_LOG("[boot] main loop started\n");
  }

  while (!g_quit) rt_run_frame();

  RT_LOG("[exit] %lu frames\n", rt_frames);
  wasm2c_tmnt2_free(&g_inst);
  wasm_rt_free();
  egl_exit();
}

static int check_assets(void) {
  static const char* required[] = {"TMNT.DAT", "TMNTE.DAT", "WaveData.bin", "Common/Fonts/MainFont.met"};
  for (size_t i = 0; i < sizeof required / sizeof *required; i++) {
    char path[256];
    struct stat st;
    snprintf(path, sizeof path, "%s/%s", GAME_DIR, required[i]);
    if (stat(path, &st) != 0) {
      consoleInit(NULL);
      printf("TMNT 2: Battle Nexus\n\nMissing file:\n  %s\n\n", path);
      printf("Copy the PC game files into %s/\nPress + to exit.\n", GAME_DIR);
      while (appletMainLoop()) {
        padUpdate(&g_padstate);
        if (padGetButtonsDown(&g_padstate) & HidNpadButton_Plus) break;
        consoleUpdate(NULL);
      }
      consoleExit(NULL);
      return 0;
    }
  }
  return 1;
}

int main(int argc, char** argv) {
  // Keep the previous run's log: after a crash people tend to relaunch before sending it.
  remove(GAME_DIR "/log_prev.txt");
  rename(GAME_DIR "/log.txt", GAME_DIR "/log_prev.txt");
  freopen(GAME_DIR "/log.txt", "w", stderr);
  setvbuf(stderr, NULL, _IONBF, 0);

  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&g_padstate);

  if (!check_assets()) return 0;

  appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);

  Thread thread;
  Result rc = threadCreate(&thread, game_thread, NULL, NULL, GAME_STACK_SIZE, 0x2C, -2);
  if (R_FAILED(rc)) {
    RT_LOG("[boot] threadCreate failed: 0x%x\n", rc);
    return 1;
  }
  threadStart(&thread);
  threadWaitForExit(&thread);
  threadClose(&thread);
  return g_exit_code;
}
