// Display-side imports shared by every platform: emscripten EGL emulation handles,
// the WebGL context calls and canvas/screen sizes. The real EGL objects (Switch) are
// owned by the platform layer; the game only ever sees these fixed handles.
#include <stdlib.h>

#include "rt.h"

#define EGL_DPY 62000
#define EGL_CFG 62002
#define EGL_CTX 62004
#define EGL_SURF 62006

unsigned long rt_frames = 0;

/* ---------------- EGL ---------------- */
u32 w2c_a_Wa(struct w2c_a* env, u32 native) { return EGL_DPY; }                          // eglGetDisplay
u32 w2c_a_Va(struct w2c_a* env, u32 dpy, u32 major, u32 minor) {                          // eglInitialize
  if (major) RT_U32(major) = 1;
  if (minor) RT_U32(minor) = 4;
  return 1;
}
u32 w2c_a_Ua(struct w2c_a* env, u32 api) { return 1; }                                    // eglBindAPI
u32 w2c_a_Ta(struct w2c_a* env, u32 dpy, u32 attrs, u32 configs, u32 size, u32 num) {     // eglChooseConfig
  if (configs && size) RT_U32(configs) = EGL_CFG;
  if (num) RT_U32(num) = 1;
  return 1;
}
u32 w2c_a_e(struct w2c_a* env, u32 dpy, u32 cfg, u32 attr, u32 out) {                     // eglGetConfigAttrib
  u32 v = 0;
  switch (attr) {
    case 12320: v = 32; break;                      // BUFFER_SIZE
    case 12321: case 12322: case 12323: case 12324: v = 8; break;  // A/B/G/R
    case 12325: v = 24; break;                      // DEPTH
    case 12326: v = 8; break;                       // STENCIL
    case 12328: v = EGL_CFG; break;                 // CONFIG_ID
    case 12352: v = 4; break;                       // RENDERABLE_TYPE = ES2
  }
  RT_U32(out) = v;
  return 1;
}
u32 w2c_a_Ra(struct w2c_a* env, u32 dpy, u32 cfg, u32 share, u32 attrs) { return EGL_CTX; }  // eglCreateContext
u32 w2c_a_Sa(struct w2c_a* env, u32 dpy, u32 cfg, u32 win, u32 attrs) { return EGL_SURF; } // eglCreateWindowSurface
u32 w2c_a_i(struct w2c_a* env, u32 dpy, u32 draw, u32 read, u32 ctx) { return 1; }         // eglMakeCurrent
u32 w2c_a_k(struct w2c_a* env) { return EGL_CTX; }                                        // eglGetCurrentContext
u32 w2c_a_l(struct w2c_a* env, u32 which) { return EGL_SURF; }                            // eglGetCurrentSurface
u32 w2c_a_ba(struct w2c_a* env, u32 dpy, u32 ctx) { return 1; }                           // eglDestroyContext
u32 w2c_a_aa(struct w2c_a* env, u32 dpy, u32 surf) { return 1; }                          // eglDestroySurface
u32 w2c_a_Xa(struct w2c_a* env, u32 dpy) { return 1; }                                    // eglTerminate
u32 w2c_a_Qa(struct w2c_a* env, u32 dpy, u32 surf) {                                       // eglSwapBuffers
  rt_frames++;
  if (!rt_platform_frame()) {
    RT_LOG("[exit] platform asked to quit\n");
    exit(0);
  }
  return 1;
}

/* ---------------- WebGL context ---------------- */
u32 w2c_a_Ha(struct w2c_a* env, u32 target, u32 attrs) { return 1; }  // emscripten_webgl_create_context
u32 w2c_a_Z(struct w2c_a* env, u32 ctx) { return 0; }                 // emscripten_webgl_make_context_current

/* ---------------- HTML5 / canvas ---------------- */
u32 w2c_a_B(struct w2c_a* env, u32 target, u32 w, u32 h) {                  // get_canvas_element_size
  RT_I32(w) = 1280;
  RT_I32(h) = 720;
  return 0;
}
u32 w2c_a_o(struct w2c_a* env, u32 target, u32 w, u32 h) {                  // get_element_css_size
  double dw = 1280, dh = 720;
  memcpy(RT_PTR(w), &dw, 8);
  memcpy(RT_PTR(h), &dh, 8);
  return 0;
}
void w2c_a_Ga(struct w2c_a* env, u32 w, u32 h) {                            // get_screen_size
  RT_I32(w) = 1280;
  RT_I32(h) = 720;
}
u32 w2c_a_x(struct w2c_a* env) { return 1; }
u32 w2c_a_xa(struct w2c_a* env) { return 1; }                               // on main thread
