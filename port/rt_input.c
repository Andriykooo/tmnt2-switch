// Gamepad input. The web build reads controllers through the HTML5 Gamepad API (via SDL3),
// so the platform pad is presented as one "standard" gamepad: Xbox 360 ids, standard
// mapping, buttons in W3C order.
#include "rt.h"

// EmscriptenGamepadEvent layout, see Zd() in the glue.
#define GP_TIMESTAMP 0
#define GP_NUM_AXES 8
#define GP_NUM_BUTTONS 12
#define GP_AXES 16
#define GP_ANALOG 528
#define GP_DIGITAL 1040
#define GP_CONNECTED 1104
#define GP_INDEX 1108
#define GP_ID 1112
#define GP_MAPPING 1176
#define GP_SIZE 1240

#define EVT_GAMEPAD_CONNECTED 26

static const char* kPadId = "Xbox 360 Controller (XInput STANDARD GAMEPAD Vendor: 045e Product: 028e)";

// EmscriptenKeyboardEvent layout, see ye() in the glue.
#define KB_LOCATION 8
#define KB_KEYCODE 24
#define KB_WHICH 28
#define KB_KEY 32
#define KB_CODE 64
#define KB_SIZE 160

#define EVT_KEYDOWN 2
#define EVT_KEYUP 3

// On desktop web the game pauses only on the Escape key (CGameStage::IsPauseTrigger checks
// the pad's START only on mobile), so the pad's Start button is also sent as Escape.
#define PAD_START 9

static rt_pad g_pad;
static int g_pad_present = 0;
static u32 g_connect_cb = 0, g_connect_user = 0;
static int g_connect_sent = 0;
static u32 g_keydown_cb = 0, g_keydown_user = 0;
static u32 g_keyup_cb = 0, g_keyup_user = 0;

static void rt_send_key(int down, const char* name, u32 keycode) {
  u32 cb = down ? g_keydown_cb : g_keyup_cb;
  if (!cb) return;
  u32 sp = rt_stackSave();
  u32 ev = rt_stackAlloc(KB_SIZE);
  memset(RT_PTR(ev), 0, KB_SIZE);
  double ts = rt_now_ms();
  memcpy(RT_PTR(ev), &ts, 8);
  RT_U32(ev + KB_KEYCODE) = keycode;
  RT_U32(ev + KB_WHICH) = keycode;
  strcpy((char*)RT_PTR(ev + KB_KEY), name);
  strcpy((char*)RT_PTR(ev + KB_CODE), name);
  w2c_tmnt2_0x5Fe(&g_inst, cb, down ? EVT_KEYDOWN : EVT_KEYUP, ev, down ? g_keydown_user : g_keyup_user);
  rt_stackRestore(sp);
}

static void rt_fill_gamepad_event(u32 out) {
  memset(RT_PTR(out), 0, GP_SIZE);
  double ts = rt_now_ms();
  memcpy(RT_PTR(out + GP_TIMESTAMP), &ts, 8);
  RT_I32(out + GP_NUM_AXES) = RT_PAD_AXES;
  RT_I32(out + GP_NUM_BUTTONS) = RT_PAD_BUTTONS;
  for (int i = 0; i < RT_PAD_AXES; i++) {
    double v = g_pad.axes[i];
    memcpy(RT_PTR(out + GP_AXES + 8 * i), &v, 8);
  }
  for (int i = 0; i < RT_PAD_BUTTONS; i++) {
    double v = g_pad.buttons[i] ? 1.0 : 0.0;
    memcpy(RT_PTR(out + GP_ANALOG + 8 * i), &v, 8);
    *(uint8_t*)RT_PTR(out + GP_DIGITAL + i) = g_pad.buttons[i] ? 1 : 0;
  }
  *(uint8_t*)RT_PTR(out + GP_CONNECTED) = 1;
  RT_I32(out + GP_INDEX) = 0;
  strcpy((char*)RT_PTR(out + GP_ID), kPadId);
  strcpy((char*)RT_PTR(out + GP_MAPPING), "standard");
}

// Called once per frame by the platform after it has read the controller.
void rt_input_update(const rt_pad* pad) {
  if (pad->buttons[PAD_START] != g_pad.buttons[PAD_START]) rt_send_key(pad->buttons[PAD_START], "Escape", 27);
  g_pad = *pad;
  g_pad_present = 1;
  // SDL registers its "gamepadconnected" listener early; announce the pad once.
  if (g_connect_cb && !g_connect_sent) {
    g_connect_sent = 1;
    u32 sp = rt_stackSave();
    u32 ev = rt_stackAlloc(GP_SIZE);
    rt_fill_gamepad_event(ev);
    w2c_tmnt2_0x5Fe(&g_inst, g_connect_cb, EVT_GAMEPAD_CONNECTED, ev, g_connect_user);
    rt_stackRestore(sp);
  }
}

// A: emscripten_set_gamepadconnected_callback(userData, useCapture, callback, thread)
u32 w2c_a_A(struct w2c_a* env, u32 user, u32 capture, u32 cb, u32 thread) {
  g_connect_cb = cb;
  g_connect_user = user;
  return 0;
}

// Q / P: emscripten_set_keydown_callback / keyup (target, userData, useCapture, callback, thread)
u32 w2c_a_Q(struct w2c_a* env, u32 target, u32 user, u32 capture, u32 cb, u32 thread) {
  g_keydown_cb = cb;
  g_keydown_user = user;
  return 0;
}
u32 w2c_a_P(struct w2c_a* env, u32 target, u32 user, u32 capture, u32 cb, u32 thread) {
  g_keyup_cb = cb;
  g_keyup_user = user;
  return 0;
}

// $: emscripten_sample_gamepad_data
u32 w2c_a_0x24(struct w2c_a* env) { return 0; }

// Ja: emscripten_get_num_gamepads
u32 w2c_a_Ja(struct w2c_a* env) { return g_pad_present ? 1 : 0; }

// _: emscripten_get_gamepad_status(index, out)
u32 w2c_a_0x5F(struct w2c_a* env, u32 index, u32 out) {
  if (index != 0 || !g_pad_present) return (u32)-5;
  rt_fill_gamepad_event(out);
  return 0;
}

// La / Ma / Ka: product id, vendor id, "is xinput" for SDL's controller database
u32 w2c_a_La(struct w2c_a* env, u32 index) { return 0x028e; }
u32 w2c_a_Ma(struct w2c_a* env, u32 index) { return 0x045e; }
u32 w2c_a_Ka(struct w2c_a* env, u32 index) { return 1; }
