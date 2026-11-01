// Core imports: invoke_* (setjmp/longjmp), time, timers, EM_ASM, WorkerFS, main loop, exit.
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <time.h>

#include "rt.h"

int rt_verbose = 0;

/* ------------------------------------------------------------------ */
/* invoke_* / emscripten_throw_longjmp                                 */
/* ------------------------------------------------------------------ */
// Emscripten implements setjmp/longjmp with JS exceptions: every call that may longjmp is
// routed through invoke_X, which catches the throw, restores the stack and calls setThrew.
// Natively the same shape is a host setjmp around the call and a longjmp from the throw.

#define RT_MAX_INVOKE_DEPTH 256

typedef struct {
  jmp_buf buf;
  uint32_t call_depth;
} rt_invoke_frame;

static rt_invoke_frame rt_invoke_stack[RT_MAX_INVOKE_DEPTH];
static int rt_invoke_top = 0;

#if WASM_RT_STACK_DEPTH_COUNT
#define RT_SAVE_DEPTH() (wasm_rt_call_stack_depth)
#define RT_RESTORE_DEPTH(d) (wasm_rt_call_stack_depth = (d))
#else
#define RT_SAVE_DEPTH() 0
#define RT_RESTORE_DEPTH(d) ((void)(d))
#endif

#define RT_INVOKE(call)                                                   \
  uint32_t sp = rt_stackSave();                                           \
  if (rt_invoke_top >= RT_MAX_INVOKE_DEPTH) {                             \
    RT_LOG("invoke stack overflow\n");                                    \
    abort();                                                              \
  }                                                                       \
  rt_invoke_frame* frame = &rt_invoke_stack[rt_invoke_top++];             \
  frame->call_depth = RT_SAVE_DEPTH();                                    \
  if (setjmp(frame->buf) == 0) {                                          \
    call;                                                                 \
    rt_invoke_top--;                                                      \
  } else {                                                                \
    rt_invoke_top--;                                                      \
    RT_RESTORE_DEPTH(frame->call_depth);                                  \
    rt_stackRestore(sp);                                                  \
    rt_setThrew(1, 0);                                                    \
  }

// Za: _emscripten_throw_longjmp
void w2c_a_Za(struct w2c_a* env) {
  if (rt_invoke_top == 0) {
    RT_LOG("longjmp with no invoke frame\n");
    abort();
  }
  longjmp(rt_invoke_stack[rt_invoke_top - 1].buf, 1);
}

// a: invoke_vii
void w2c_a_a(struct w2c_a* env, u32 idx, u32 a0, u32 a1) {
  RT_INVOKE(w2c_tmnt2_bf(&g_inst, idx, a0, a1));
}

// b: invoke_viii
void w2c_a_b(struct w2c_a* env, u32 idx, u32 a0, u32 a1, u32 a2) {
  RT_INVOKE(w2c_tmnt2_0x24e(&g_inst, idx, a0, a1, a2));
}

// j: invoke_vi
void w2c_a_j(struct w2c_a* env, u32 idx, u32 a0) {
  RT_INVOKE(w2c_tmnt2_af(&g_inst, idx, a0));
}

// f: invoke_iii
u32 w2c_a_f(struct w2c_a* env, u32 idx, u32 a0, u32 a1) {
  u32 result = 0;
  RT_INVOKE(result = w2c_tmnt2_Ze(&g_inst, idx, a0, a1));
  return result;
}

// C: invoke_iiii
u32 w2c_a_C(struct w2c_a* env, u32 idx, u32 a0, u32 a1, u32 a2) {
  u32 result = 0;
  RT_INVOKE(result = w2c_tmnt2_0x5Fe(&g_inst, idx, a0, a1, a2));
  return result;
}

// ca: invoke_ii
u32 w2c_a_ca(struct w2c_a* env, u32 idx, u32 a0) {
  u32 result = 0;
  RT_INVOKE(result = w2c_tmnt2_Ye(&g_inst, idx, a0));
  return result;
}

// _a: invoke_i
u32 w2c_a_0x5Fa(struct w2c_a* env, u32 idx) {
  u32 result = 0;
  RT_INVOKE(result = w2c_tmnt2_lf(&g_inst, idx));
  return result;
}

// $a: dynCall_vi passthrough
void w2c_a_0x24a(struct w2c_a* env, u32 idx, u32 a0) {
  w2c_tmnt2_af(&g_inst, idx, a0);
}

/* ------------------------------------------------------------------ */
/* time                                                                */
/* ------------------------------------------------------------------ */

// q: performance.now
f64 w2c_a_q(struct w2c_a* env) { return rt_now_ms(); }

// rb: Date.now
f64 w2c_a_rb(struct w2c_a* env) { return rt_epoch_ms(); }

// w: devicePixelRatio
f64 w2c_a_w(struct w2c_a* env) { return 1.0; }

// tb: clock_time_get(clock_id, precision, out_u64_ptr)
u32 w2c_a_tb(struct w2c_a* env, u32 clock_id, u64 precision, u32 out) {
  if (clock_id > 3) return 28;
  double ms = clock_id == 0 ? rt_epoch_ms() : rt_now_ms();
  uint64_t ns = (uint64_t)llround(ms * 1e6);
  memcpy(RT_PTR(out), &ns, 8);
  return 0;
}

// eb: _localtime_js(time_t seconds, struct tm*)
void w2c_a_eb(struct w2c_a* env, u64 seconds, u32 tm_ptr) {
  time_t t = (time_t)(int64_t)seconds;
  struct tm local = *localtime(&t);
  int32_t* out = (int32_t*)RT_PTR(tm_ptr);
  out[0] = local.tm_sec;
  out[1] = local.tm_min;
  out[2] = local.tm_hour;
  out[3] = local.tm_mday;
  out[4] = local.tm_mon;
  out[5] = local.tm_year;
  out[6] = local.tm_wday;
  out[7] = local.tm_yday;
  out[8] = local.tm_isdst;
  out[9] = 0;  // tm_gmtoff
  RT_U32(tm_ptr + 40) = 0;  // tm_zone
}

// fb: _tzset_js(timezone*, daylight*, std_name, dst_name)
void w2c_a_fb(struct w2c_a* env, u32 timezone, u32 daylight, u32 std_name, u32 dst_name) {
  RT_U32(timezone) = 0;
  RT_I32(daylight) = 0;
  strcpy((char*)RT_PTR(std_name), "UTC");
  strcpy((char*)RT_PTR(dst_name), "UTC");
}

/* ------------------------------------------------------------------ */
/* timers (_setitimer_js -> _emscripten_timeout)                       */
/* ------------------------------------------------------------------ */

static double rt_timer_due[3] = {0, 0, 0};

// cb: _setitimer_js(which, timeout_ms)
u32 w2c_a_cb(struct w2c_a* env, u32 which, f64 timeout_ms) {
  if (which > 2) return 28;
  rt_timer_due[which] = timeout_ms > 0 ? rt_now_ms() + timeout_ms : 0;
  return 0;
}

static void rt_pump_timers(void) {
  for (u32 i = 0; i < 3; i++) {
    if (rt_timer_due[i] != 0 && rt_now_ms() >= rt_timer_due[i]) {
      rt_timer_due[i] = 0;
      rt_emscripten_timeout(i, rt_now_ms());
    }
  }
}

/* ------------------------------------------------------------------ */
/* WorkerFS: the game asks JS to read TMNT.DAT & co. asynchronously     */
/* ------------------------------------------------------------------ */

enum { RT_ASYNC_READY, RT_ASYNC_MOUNT, RT_ASYNC_DATA };

typedef struct {
  int kind;
  char path[256];
  uint32_t offset, size, opid, param;
  int size_only;
} rt_async_op;

#define RT_MAX_ASYNC 256
static rt_async_op rt_async_queue[RT_MAX_ASYNC];
static int rt_async_head = 0, rt_async_tail = 0;

static void rt_async_push(rt_async_op op) {
  int next = (rt_async_tail + 1) % RT_MAX_ASYNC;
  if (next == rt_async_head) {
    RT_LOG("async queue full\n");
    abort();
  }
  rt_async_queue[rt_async_tail] = op;
  rt_async_tail = next;
}

// "/wfs/TMNT.DAT" -> "<asset dir>/TMNT.DAT"
static void rt_workerfs_path(const char* wasm_path, char* out, size_t out_size) {
  const char* name = strrchr(wasm_path, '/');
  snprintf(out, out_size, "%s/%s", rt_asset_dir(), name ? name + 1 : wasm_path);
}

static void rt_async_run(const rt_async_op* op) {
  switch (op->kind) {
    case RT_ASYNC_READY:
      rt_onWorkerFSReady();
      return;
    case RT_ASYNC_MOUNT:
      rt_onWorkerFSMountReady();
      return;
    case RT_ASYNC_DATA:
      break;
  }

  char host[512];
  rt_workerfs_path(op->path, host, sizeof host);
  FILE* f = fopen(host, "rb");
  if (!f) {
    RT_LOG("[wfs] missing %s\n", host);
    rt_onWorkerFSDataReady(0, 0, op->opid, op->param);
    return;
  }
  fseek(f, 0, SEEK_END);
  uint32_t file_size = (uint32_t)ftell(f);

  if (op->size_only) {
    fclose(f);
    rt_onWorkerFSDataReady(0, file_size, op->opid, op->param);
    return;
  }

  uint32_t size = op->size;
  if (op->offset >= file_size) size = 0;
  else if (size > file_size - op->offset) size = file_size - op->offset;

  uint32_t buf = size ? rt_malloc(size) : 0;
  if (size && !buf) {
    fclose(f);
    RT_LOG("[wfs] malloc(%u) failed\n", size);
    return;
  }
  fseek(f, op->offset, SEEK_SET);
  size_t got = size ? fread(RT_PTR(buf), 1, size, f) : 0;
  fclose(f);
  rt_onWorkerFSDataReady(buf, (uint32_t)got, op->opid, op->param);
  if (buf) rt_free(buf);
}

void rt_pump_async(void) {
  rt_pump_timers();
  // Callbacks may queue further reads; drain only what was queued on entry.
  int end = rt_async_tail;
  while (rt_async_head != end) {
    rt_async_op op = rt_async_queue[rt_async_head];
    rt_async_head = (rt_async_head + 1) % RT_MAX_ASYNC;
    rt_async_run(&op);
  }
}

/* ------------------------------------------------------------------ */
/* EM_ASM                                                              */
/* ------------------------------------------------------------------ */

typedef union {
  int32_t i;
  double d;
  int64_t j;
} rt_asm_arg;

// Mirrors emscripten's readEmAsmArgs: sig chars 'i'/'p' are 4 bytes, 'd'/'j' 8-byte aligned.
static int rt_read_asm_args(u32 sig, u32 buf, rt_asm_arg* args, int max) {
  const char* s = RT_STR(sig);
  int n = 0;
  for (; *s && n < max; s++, n++) {
    int wide = *s == 'd' || *s == 'j';
    if (wide && (buf & 7)) buf += 4;
    if (*s == 'd') memcpy(&args[n].d, RT_PTR(buf), 8);
    else if (*s == 'j') memcpy(&args[n].j, RT_PTR(buf), 8);
    else args[n].i = RT_I32(buf);
    buf += wide ? 8 : 4;
  }
  return n;
}

#define RT_SCREEN_W 1280
#define RT_SCREEN_H 720

static double rt_em_asm(u32 code, u32 sig, u32 argbuf) {
  rt_asm_arg a[8] = {0};
  int argc = rt_read_asm_args(sig, argbuf, a, 8);
  rt_async_op op = {0};
  if (rt_verbose) RT_LOG("[em_asm] %u sig=%s argc=%d a0=%d\n", code, RT_STR(sig), argc, a[0].i);

  switch (code) {
    case 386645:  // mkdir + mount IDBFS
      rt_fs_mkdir(RT_STR(a[0].i));
      return 0;
    case 386716:  // unmount IDBFS
      return 0;
    case 386750:  // FS.syncfs -> ccall(name, ok); the only callback is webIDBFSSyncCallback
      w2c_tmnt2_te(&g_inst, 1);
      return 0;
    case 386899:  // remote FS: sync HEAD
    case 387225:  // remote FS: async HEAD
    case 388065:  // remote FS: abort
    case 388253:  // terminateWorkerFS
      return 0;
    case 388186:  // initializeWorkerFS
      op.kind = RT_ASYNC_READY;
      rt_async_push(op);
      return 0;
    case 388212:  // postWorkerFSMount(path)
      op.kind = RT_ASYNC_MOUNT;
      rt_async_push(op);
      return 0;
    case 388278:  // postWorkerFSGetSize(fname, opid, param)
      op.kind = RT_ASYNC_DATA;
      op.size_only = 1;
      snprintf(op.path, sizeof op.path, "%s", RT_STR(a[0].i));
      op.opid = a[1].i;
      op.param = a[2].i;
      rt_async_push(op);
      return 0;
    case 388329:  // postWorkerFSRead(fname, offset, size, opid, param)
      op.kind = RT_ASYNC_DATA;
      snprintf(op.path, sizeof op.path, "%s", RT_STR(a[0].i));
      op.offset = a[1].i;
      op.size = a[2].i;
      op.opid = a[3].i;
      op.param = a[4].i;
      rt_async_push(op);
      return 0;
    case 388385:  // alert(msg) then ccall(webMsgBoxCallback, 0)
      RT_LOG("[msgbox] %s\n", RT_STR(a[0].i));
      w2c_tmnt2_ze(&g_inst, 0);
      return 0;
    case 394593:  // alert(title + text)
      RT_LOG("[alert] %s: %s\n", RT_STR(a[0].i), RT_STR(a[1].i));
      return 0;
    case 389884:  // prefers dark
      return 1;
    case 390346: case 391083: case 399265: case 399342: case 399403:
      return RT_SCREEN_W;
    case 390376: case 391113: case 399303: case 399372: case 399433:
      return RT_SCREEN_H;
    case 398885:  // canvas focused
    case 399051:  // document.hasFocus
      return 1;
    default:      // DOM, mouse, cursor, vibration, mobile detection: nothing to do natively
      return 0;
  }
}

// W / d / c / Fa: emscripten_asm_const_{double,int,...}(code, sig, argbuf)
f64 w2c_a_W(struct w2c_a* env, u32 code, u32 sig, u32 argbuf) { return rt_em_asm(code, sig, argbuf); }
u32 w2c_a_d(struct w2c_a* env, u32 code, u32 sig, u32 argbuf) { return (u32)(int32_t)rt_em_asm(code, sig, argbuf); }
u32 w2c_a_c(struct w2c_a* env, u32 code, u32 sig, u32 argbuf) { return (u32)(int32_t)rt_em_asm(code, sig, argbuf); }
u32 w2c_a_Fa(struct w2c_a* env, u32 code, u32 sig, u32 argbuf) { return (u32)(int32_t)rt_em_asm(code, sig, argbuf); }

/* ------------------------------------------------------------------ */
/* main loop, sleep, exit                                              */
/* ------------------------------------------------------------------ */

static u32 rt_loop_func = 0, rt_loop_arg = 0;
static int rt_loop_set = 0;
jmp_buf rt_main_exit;
uint32_t rt_main_exit_depth;

// Da: emscripten_set_main_loop_arg(func, arg, fps, simulate_infinite_loop)
void w2c_a_Da(struct w2c_a* env, u32 func, u32 arg, u32 fps, u32 simulate_infinite) {
  rt_loop_func = func;
  rt_loop_arg = arg;
  rt_loop_set = 1;
  RT_LOG("[loop] main loop set func=%u arg=%u fps=%d infinite=%u\n", func, arg, (int)fps, simulate_infinite);
  if (simulate_infinite) {
    // JS throws "unwind" to leave main(); natively jump back to the harness.
    RT_RESTORE_DEPTH(rt_main_exit_depth);
    longjmp(rt_main_exit, 1);
  }
}

// y: emscripten_set_main_loop_timing
u32 w2c_a_y(struct w2c_a* env, u32 mode, u32 value) { return 0; }

// g: emscripten_sleep (asyncify). Natively: deliver pending work, then really sleep.
void w2c_a_g(struct w2c_a* env, u32 ms) {
  static unsigned long calls;
  static int module_ready_sent = 0;
  if (rt_verbose && (calls++ % 500) == 0) RT_LOG("[sleep] %lu calls, last %u ms\n", calls, ms);
  // In the browser main() yields at its first sleep, the glue finishes loading and its
  // postRun hook calls _onMainModuleReady. Natively that moment is this first sleep.
  if (!module_ready_sent) {
    module_ready_sent = 1;
    RT_LOG("[boot] onMainModuleReady\n");
    rt_onMainModuleReady();
  }
  rt_pump_async();
  if (ms) rt_sleep_ms(ms);
}

// ab: proc_exit / Ya: emscripten_force_exit
void w2c_a_ab(struct w2c_a* env, u32 code) {
  RT_LOG("[exit] proc_exit(%u)\n", code);
  exit((int)code);
}
void w2c_a_Ya(struct w2c_a* env, u32 code) {
  RT_LOG("[exit] force_exit(%u)\n", code);
  exit((int)code);
}

// ub: abort / db: abort on OOM
void w2c_a_ub(struct w2c_a* env) {
  RT_LOG("[abort]\n");
  abort();
}
// db: emscripten_resize_heap - the web build has a fixed heap, so growth always fails
u32 w2c_a_db(struct w2c_a* env, u32 size) {
  RT_LOG("[heap] resize to %u refused\n", size);
  return 0;
}

// sb: emscripten_log / console output
void w2c_a_sb(struct w2c_a* env, u32 flags, u32 str) { RT_LOG("[log] %s\n", RT_STR(str)); }

// Ba: document.title
void w2c_a_Ba(struct w2c_a* env, u32 title) { RT_LOG("[title] %s\n", RT_STR(title)); }

int rt_run_frame(void) {
  rt_pump_async();
  if (!rt_loop_set) return 1;
  w2c_tmnt2_af(&g_inst, rt_loop_func, rt_loop_arg);
  return 1;
}
