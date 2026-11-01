// Headless GL answers for the PC test harness: names, compile/link status and limits,
// so the game believes it has a working GLES2 context. Nothing is drawn.
#include "rt.h"

/* ---------------- GL object names ---------------- */
static u32 rt_next_name = 1;
static void rt_gen_names(u32 n, u32 out) {
  for (u32 i = 0; i < n; i++) RT_U32(out + 4 * i) = rt_next_name++;
}
void w2c_a_kd(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenBuffers
void w2c_a_id(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenFramebuffers
void w2c_a_hd(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenRenderbuffers
void w2c_a_gd(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenTextures
void w2c_a_ne(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenVertexArrays
void w2c_a_ua(struct w2c_a* env, u32 n, u32 out) { rt_gen_names(n, out); }  // glGenQueriesEXT
u32 w2c_a_Id(struct w2c_a* env) { return rt_next_name++; }                  // glCreateProgram
u32 w2c_a_Hd(struct w2c_a* env, u32 type) { return rt_next_name++; }       // glCreateShader

/* ---------------- GL queries ---------------- */
void w2c_a_Uc(struct w2c_a* env, u32 shader, u32 pname, u32 out) {          // glGetShaderiv
  if (out) RT_I32(out) = pname == 35713 ? 1 : 0;                            // COMPILE_STATUS
}
void w2c_a_Xc(struct w2c_a* env, u32 program, u32 pname, u32 out) {         // glGetProgramiv
  if (out) RT_I32(out) = (pname == 35714 || pname == 35715) ? 1 : 0;        // LINK/VALIDATE_STATUS
}
u32 w2c_a_Ud(struct w2c_a* env, u32 target) { return 36053; }               // glCheckFramebufferStatus
u32 w2c_a_0x24c(struct w2c_a* env) { return 0; }                            // glGetError

static u32 rt_uniform_next = 1;
u32 w2c_a_Lc(struct w2c_a* env, u32 program, u32 name) { return rt_uniform_next++; }  // glGetUniformLocation
u32 w2c_a_cd(struct w2c_a* env, u32 program, u32 name) { return 0; }                   // glGetAttribLocation

void w2c_a_Yc(struct w2c_a* env, u32 pname, u32 out) {                      // glGetIntegerv
  if (!out) return;
  switch (pname) {
    case 3379: RT_I32(out) = 4096; break;    // MAX_TEXTURE_SIZE
    case 34921: RT_I32(out) = 16; break;     // MAX_VERTEX_ATTRIBS
    case 34930: case 35660: case 35661: RT_I32(out) = 16; break;  // texture units
    case 36347: RT_I32(out) = 256; break;    // MAX_VERTEX_UNIFORM_VECTORS
    case 36349: RT_I32(out) = 224; break;    // MAX_FRAGMENT_UNIFORM_VECTORS
    case 36348: RT_I32(out) = 15; break;     // MAX_VARYING_VECTORS
    case 34076: RT_I32(out) = 4096; break;   // MAX_CUBE_MAP_TEXTURE_SIZE
    case 34024: RT_I32(out) = 4096; break;   // MAX_RENDERBUFFER_SIZE
    case 2978:                               // VIEWPORT
      RT_I32(out) = 0; RT_I32(out + 4) = 0; RT_I32(out + 8) = 1280; RT_I32(out + 12) = 720;
      break;
    default: RT_I32(out) = 0;
  }
}

static u32 rt_gl_strings[8];
u32 w2c_a_Qc(struct w2c_a* env, u32 name) {                                 // glGetString
  const char* s;
  int slot;
  switch (name) {
    case 7936: s = "Mesa"; slot = 0; break;                           // VENDOR
    case 7937: s = "Headless"; slot = 1; break;                       // RENDERER
    case 7938: s = "OpenGL ES 2.0 WebGL 1.0"; slot = 2; break;       // VERSION
    case 35724: s = "OpenGL ES GLSL ES 1.00 WebGL GLSL ES 1.0"; slot = 3; break;
    case 7939: s = ""; slot = 4; break;                               // EXTENSIONS
    default: return 0;
  }
  if (!rt_gl_strings[slot]) {
    size_t len = strlen(s) + 1;
    rt_gl_strings[slot] = rt_malloc((u32)len);
    memcpy(RT_PTR(rt_gl_strings[slot]), s, len);
  }
  return rt_gl_strings[slot];
}

