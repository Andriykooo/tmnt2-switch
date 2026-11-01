// WebGL imports mapped onto native OpenGL ES (a GLES 3 context, which also runs the
// game's GLES2 shaders and has VAOs/instancing in core).
//
// Differences from the emscripten glue that matter here:
//  * Object names are native GL names - the wasm side never sees WebGL objects.
//  * Pointers are wasm addresses; RT_PTR turns them into host pointers.
//  * Client-side vertex/index arrays (no buffer bound) are native GLES behaviour, so the
//    glue's emulation (copying into temp buffers) is replaced by passing the pointer.
#include <GLES3/gl3.h>
#include <stdlib.h>

#include "rt.h"

#define P(addr) ((addr) ? RT_PTR(addr) : NULL)

/* ---------------- state ---------------- */
void w2c_a_he(struct w2c_a* e, u32 t) { glActiveTexture(t); }
void w2c_a_ae(struct w2c_a* e, f32 r, f32 g, f32 b, f32 a) { glBlendColor(r, g, b, a); }
void w2c_a_0x24d(struct w2c_a* e, u32 m) { glBlendEquation(m); }
void w2c_a_0x5Fd(struct w2c_a* e, u32 rgb, u32 a) { glBlendEquationSeparate(rgb, a); }
void w2c_a_Zd(struct w2c_a* e, u32 s, u32 d) { glBlendFunc(s, d); }
void w2c_a_Yd(struct w2c_a* e, u32 sr, u32 dr, u32 sa, u32 da) { glBlendFuncSeparate(sr, dr, sa, da); }
void w2c_a_Td(struct w2c_a* e, u32 mask) { glClear(mask); }
void w2c_a_Sd(struct w2c_a* e, f32 r, f32 g, f32 b, f32 a) { glClearColor(r, g, b, a); }
void w2c_a_Rd(struct w2c_a* e, f32 d) { glClearDepthf(d); }
void w2c_a_Qd(struct w2c_a* e, u32 s) { glClearStencil((GLint)s); }
void w2c_a_Pd(struct w2c_a* e, u32 r, u32 g, u32 b, u32 a) { glColorMask(!!r, !!g, !!b, !!a); }
void w2c_a_Gd(struct w2c_a* e, u32 m) { glCullFace(m); }
void w2c_a_zd(struct w2c_a* e, u32 f) { glDepthFunc(f); }
void w2c_a_yd(struct w2c_a* e, u32 f) { glDepthMask(!!f); }
void w2c_a_xd(struct w2c_a* e, f32 n, f32 f) { glDepthRangef(n, f); }
void w2c_a_vd(struct w2c_a* e, u32 cap) { glDisable(cap); }
void w2c_a_rd(struct w2c_a* e, u32 cap) { glEnable(cap); }
u32 w2c_a_Fc(struct w2c_a* e, u32 cap) { return glIsEnabled(cap); }
void w2c_a_ld(struct w2c_a* e, u32 m) { glFrontFace(m); }
void w2c_a_Hc(struct w2c_a* e, u32 t, u32 m) { glHint(t, m); }
void w2c_a_zc(struct w2c_a* e, f32 w) { glLineWidth(w); }
void w2c_a_xc(struct w2c_a* e, u32 p, u32 v) { glPixelStorei(p, (GLint)v); }
void w2c_a_wc(struct w2c_a* e, f32 f, f32 u) { glPolygonOffset(f, u); }
void w2c_a_sc(struct w2c_a* e, f32 v, u32 inv) { glSampleCoverage(v, !!inv); }
void w2c_a_rc(struct w2c_a* e, u32 x, u32 y, u32 w, u32 h) { glScissor((GLint)x, (GLint)y, w, h); }
void w2c_a_Ab(struct w2c_a* e, u32 x, u32 y, u32 w, u32 h) { glViewport((GLint)x, (GLint)y, w, h); }
void w2c_a_oc(struct w2c_a* e, u32 f, u32 r, u32 m) { glStencilFunc(f, (GLint)r, m); }
void w2c_a_Pa(struct w2c_a* e, u32 f, u32 r, u32 m) { glStencilFunc(f, (GLint)r, m); }
void w2c_a_nc(struct w2c_a* e, u32 face, u32 f, u32 r, u32 m) { glStencilFuncSeparate(face, f, (GLint)r, m); }
void w2c_a_mc(struct w2c_a* e, u32 m) { glStencilMask(m); }
void w2c_a_lc(struct w2c_a* e, u32 face, u32 m) { glStencilMaskSeparate(face, m); }
void w2c_a_kc(struct w2c_a* e, u32 f, u32 zf, u32 zp) { glStencilOp(f, zf, zp); }
void w2c_a_Oa(struct w2c_a* e, u32 f, u32 zf, u32 zp) { glStencilOp(f, zf, zp); }
void w2c_a_jc(struct w2c_a* e, u32 face, u32 f, u32 zf, u32 zp) { glStencilOpSeparate(face, f, zf, zp); }
void w2c_a_pd(struct w2c_a* e) { glFinish(); }
void w2c_a_od(struct w2c_a* e) { glFlush(); }
u32 w2c_a_0x24c(struct w2c_a* e) { return glGetError(); }

// WebGL-only extensions with no GLES equivalent: clip control, polygon mode, offset clamp.
void w2c_a_yb(struct w2c_a* e, u32 a, u32 b) {}
void w2c_a_xb(struct w2c_a* e, u32 a, u32 b) {}
void w2c_a_zb(struct w2c_a* e, f32 a, f32 b, f32 c) {}
void w2c_a_qc(struct w2c_a* e, u32 a, u32 b, u32 c, u32 d, u32 f) {}  // glShaderBinary
void w2c_a_uc(struct w2c_a* e) { glReleaseShaderCompiler(); }

/* ---------------- object names ---------------- */
void w2c_a_kd(struct w2c_a* e, u32 n, u32 out) { glGenBuffers(n, (GLuint*)RT_PTR(out)); }
void w2c_a_id(struct w2c_a* e, u32 n, u32 out) { glGenFramebuffers(n, (GLuint*)RT_PTR(out)); }
void w2c_a_hd(struct w2c_a* e, u32 n, u32 out) { glGenRenderbuffers(n, (GLuint*)RT_PTR(out)); }
void w2c_a_gd(struct w2c_a* e, u32 n, u32 out) { glGenTextures(n, (GLuint*)RT_PTR(out)); }
void w2c_a_ne(struct w2c_a* e, u32 n, u32 out) { glGenVertexArrays(n, (GLuint*)RT_PTR(out)); }
void w2c_a_ua(struct w2c_a* e, u32 n, u32 out) { glGenQueries(n, (GLuint*)RT_PTR(out)); }
void w2c_a_Fd(struct w2c_a* e, u32 n, u32 ids) { glDeleteBuffers(n, (GLuint*)RT_PTR(ids)); }
void w2c_a_Ed(struct w2c_a* e, u32 n, u32 ids) { glDeleteFramebuffers(n, (GLuint*)RT_PTR(ids)); }
void w2c_a_Cd(struct w2c_a* e, u32 n, u32 ids) { glDeleteRenderbuffers(n, (GLuint*)RT_PTR(ids)); }
void w2c_a_Ad(struct w2c_a* e, u32 n, u32 ids) { glDeleteTextures(n, (GLuint*)RT_PTR(ids)); }
void w2c_a_ha(struct w2c_a* e, u32 n, u32 ids) { glDeleteVertexArrays(n, (GLuint*)RT_PTR(ids)); }
void w2c_a_ta(struct w2c_a* e, u32 n, u32 ids) { glDeleteQueries(n, (GLuint*)RT_PTR(ids)); }
u32 w2c_a_Gc(struct w2c_a* e, u32 o) { return glIsBuffer(o); }
u32 w2c_a_Ec(struct w2c_a* e, u32 o) { return glIsFramebuffer(o); }
u32 w2c_a_Dc(struct w2c_a* e, u32 o) { return glIsProgram(o); }
u32 w2c_a_Cc(struct w2c_a* e, u32 o) { return glIsRenderbuffer(o); }
u32 w2c_a_Bc(struct w2c_a* e, u32 o) { return glIsShader(o); }
u32 w2c_a_Ac(struct w2c_a* e, u32 o) { return glIsTexture(o); }
u32 w2c_a_me(struct w2c_a* e, u32 o) { return glIsVertexArray(o); }
u32 w2c_a_sa(struct w2c_a* e, u32 o) { return glIsQuery(o); }

/* ---------------- bindings ---------------- */
void w2c_a_ee(struct w2c_a* e, u32 t, u32 b) { glBindBuffer(t, b); }
void w2c_a_de(struct w2c_a* e, u32 t, u32 f) { glBindFramebuffer(t, f); }
void w2c_a_ce(struct w2c_a* e, u32 t, u32 r) { glBindRenderbuffer(t, r); }
void w2c_a_be(struct w2c_a* e, u32 t, u32 tex) { glBindTexture(t, tex); }
void w2c_a_ia(struct w2c_a* e, u32 vao) { glBindVertexArray(vao); }

/* ---------------- buffers ---------------- */
void w2c_a_Xd(struct w2c_a* e, u32 t, u32 size, u32 data, u32 usage) { glBufferData(t, size, P(data), usage); }
void w2c_a_Wd(struct w2c_a* e, u32 t, u32 off, u32 size, u32 data) { glBufferSubData(t, off, size, P(data)); }
void w2c_a_ad(struct w2c_a* e, u32 t, u32 p, u32 out) { if (out) glGetBufferParameteriv(t, p, (GLint*)RT_PTR(out)); }

/* ---------------- framebuffers ---------------- */
u32 w2c_a_Ud(struct w2c_a* e, u32 t) { return glCheckFramebufferStatus(t); }
void w2c_a_nd(struct w2c_a* e, u32 t, u32 a, u32 rt, u32 rb) { glFramebufferRenderbuffer(t, a, rt, rb); }
void w2c_a_md(struct w2c_a* e, u32 t, u32 a, u32 tt, u32 tex, u32 lvl) { glFramebufferTexture2D(t, a, tt, tex, (GLint)lvl); }
void w2c_a_tc(struct w2c_a* e, u32 t, u32 f, u32 w, u32 h) { glRenderbufferStorage(t, f, w, h); }
void w2c_a_Vc(struct w2c_a* e, u32 t, u32 p, u32 out) { if (out) glGetRenderbufferParameteriv(t, p, (GLint*)RT_PTR(out)); }
void w2c_a_Zc(struct w2c_a* e, u32 t, u32 a, u32 p, u32 out) {
  if (out) glGetFramebufferAttachmentParameteriv(t, a, p, (GLint*)RT_PTR(out));
}
void w2c_a_le(struct w2c_a* e, u32 n, u32 bufs) { glDrawBuffers(n, (const GLenum*)RT_PTR(bufs)); }
void w2c_a_vc(struct w2c_a* e, u32 x, u32 y, u32 w, u32 h, u32 f, u32 t, u32 px) {
  glReadPixels((GLint)x, (GLint)y, w, h, f, t, P(px));
}

/* ---------------- textures ---------------- */
void w2c_a_ic(struct w2c_a* e, u32 t, u32 lvl, u32 ifmt, u32 w, u32 h, u32 b, u32 f, u32 ty, u32 px) {
  glTexImage2D(t, (GLint)lvl, (GLint)ifmt, w, h, (GLint)b, f, ty, P(px));
}
void w2c_a_dc(struct w2c_a* e, u32 t, u32 lvl, u32 x, u32 y, u32 w, u32 h, u32 f, u32 ty, u32 px) {
  glTexSubImage2D(t, (GLint)lvl, (GLint)x, (GLint)y, w, h, f, ty, P(px));
}
void w2c_a_Nd(struct w2c_a* e, u32 t, u32 lvl, u32 ifmt, u32 w, u32 h, u32 b, u32 size, u32 data) {
  glCompressedTexImage2D(t, (GLint)lvl, ifmt, w, h, (GLint)b, size, P(data));
}
void w2c_a_Md(struct w2c_a* e, u32 t, u32 lvl, u32 x, u32 y, u32 w, u32 h, u32 f, u32 size, u32 data) {
  glCompressedTexSubImage2D(t, (GLint)lvl, (GLint)x, (GLint)y, w, h, f, size, P(data));
}
void w2c_a_Ld(struct w2c_a* e, u32 t, u32 lvl, u32 f, u32 x, u32 y, u32 w, u32 h, u32 b) {
  glCopyTexImage2D(t, (GLint)lvl, f, (GLint)x, (GLint)y, w, h, (GLint)b);
}
void w2c_a_Jd(struct w2c_a* e, u32 t, u32 lvl, u32 xo, u32 yo, u32 x, u32 y, u32 w, u32 h) {
  glCopyTexSubImage2D(t, (GLint)lvl, (GLint)xo, (GLint)yo, (GLint)x, (GLint)y, w, h);
}
void w2c_a_jd(struct w2c_a* e, u32 t) { glGenerateMipmap(t); }
void w2c_a_hc(struct w2c_a* e, u32 t, u32 p, f32 v) { glTexParameterf(t, p, v); }
void w2c_a_gc(struct w2c_a* e, u32 t, u32 p, u32 v) { glTexParameterf(t, p, RT_F32(v)); }
void w2c_a_fc(struct w2c_a* e, u32 t, u32 p, u32 v) { glTexParameteri(t, p, (GLint)v); }
void w2c_a_ec(struct w2c_a* e, u32 t, u32 p, u32 v) { glTexParameteri(t, p, RT_I32(v)); }
void w2c_a_Pc(struct w2c_a* e, u32 t, u32 p, u32 out) { if (out) glGetTexParameterfv(t, p, (GLfloat*)RT_PTR(out)); }
void w2c_a_Oc(struct w2c_a* e, u32 t, u32 p, u32 out) { if (out) glGetTexParameteriv(t, p, (GLint*)RT_PTR(out)); }

/* ---------------- shaders & programs ---------------- */
u32 w2c_a_Id(struct w2c_a* e) { return glCreateProgram(); }
u32 w2c_a_Hd(struct w2c_a* e, u32 type) { return glCreateShader(type); }
void w2c_a_Dd(struct w2c_a* e, u32 p) { glDeleteProgram(p); }
void w2c_a_Bd(struct w2c_a* e, u32 s) { glDeleteShader(s); }
void w2c_a_ge(struct w2c_a* e, u32 p, u32 s) { glAttachShader(p, s); }
void w2c_a_wd(struct w2c_a* e, u32 p, u32 s) { glDetachShader(p, s); }
void w2c_a_Od(struct w2c_a* e, u32 s) { glCompileShader(s); }
void w2c_a_yc(struct w2c_a* e, u32 p) { glLinkProgram(p); }
void w2c_a_Kb(struct w2c_a* e, u32 p) { glValidateProgram(p); }
void w2c_a_Lb(struct w2c_a* e, u32 p) { glUseProgram(p); }
void w2c_a_fe(struct w2c_a* e, u32 p, u32 idx, u32 name) { glBindAttribLocation(p, idx, RT_STR(name)); }
u32 w2c_a_cd(struct w2c_a* e, u32 p, u32 name) { return (u32)glGetAttribLocation(p, RT_STR(name)); }
u32 w2c_a_Lc(struct w2c_a* e, u32 p, u32 name) { return (u32)glGetUniformLocation(p, RT_STR(name)); }

// glShaderSource(shader, count, strings, lengths): the arrays hold wasm pointers.
void w2c_a_pc(struct w2c_a* e, u32 shader, u32 count, u32 strings, u32 lengths) {
  const GLchar* srcs[16];
  GLint lens[16];
  if (count > 16) count = 16;
  for (u32 i = 0; i < count; i++) {
    srcs[i] = (const GLchar*)RT_PTR(RT_U32(strings + 4 * i));
    lens[i] = lengths ? RT_I32(lengths + 4 * i) : -1;
  }
  glShaderSource(shader, count, srcs, lens);
  if (rt_verbose) RT_LOG("[gl] shader %u source:\n%s\n", shader, srcs[0]);
}

void w2c_a_Uc(struct w2c_a* e, u32 s, u32 p, u32 out) {
  if (!out) return;
  glGetShaderiv(s, p, (GLint*)RT_PTR(out));
  if (p == GL_COMPILE_STATUS && !RT_I32(out)) {
    char log[1024];
    glGetShaderInfoLog(s, sizeof log, NULL, log);
    RT_LOG("[gl] shader %u failed to compile:\n%s\n", s, log);
  }
}
void w2c_a_Xc(struct w2c_a* e, u32 p, u32 pname, u32 out) {
  if (!out) return;
  glGetProgramiv(p, pname, (GLint*)RT_PTR(out));
  if (pname == GL_LINK_STATUS && !RT_I32(out)) {
    char log[1024];
    glGetProgramInfoLog(p, sizeof log, NULL, log);
    RT_LOG("[gl] program %u failed to link:\n%s\n", p, log);
  }
}
void w2c_a_Tc(struct w2c_a* e, u32 s, u32 max, u32 len, u32 buf) {
  glGetShaderInfoLog(s, max, len ? (GLsizei*)RT_PTR(len) : NULL, (GLchar*)P(buf));
}
void w2c_a_Wc(struct w2c_a* e, u32 p, u32 max, u32 len, u32 buf) {
  glGetProgramInfoLog(p, max, len ? (GLsizei*)RT_PTR(len) : NULL, (GLchar*)P(buf));
}
void w2c_a_Rc(struct w2c_a* e, u32 s, u32 max, u32 len, u32 buf) {
  glGetShaderSource(s, max, len ? (GLsizei*)RT_PTR(len) : NULL, (GLchar*)P(buf));
}
void w2c_a_Sc(struct w2c_a* e, u32 st, u32 pt, u32 range, u32 prec) {
  glGetShaderPrecisionFormat(st, pt, (GLint*)RT_PTR(range), (GLint*)RT_PTR(prec));
}
void w2c_a_dd(struct w2c_a* e, u32 p, u32 max, u32 count, u32 out) {
  glGetAttachedShaders(p, max, count ? (GLsizei*)RT_PTR(count) : NULL, (GLuint*)RT_PTR(out));
}
void w2c_a_fd(struct w2c_a* e, u32 p, u32 i, u32 max, u32 len, u32 size, u32 type, u32 name) {
  glGetActiveAttrib(p, i, max, len ? (GLsizei*)RT_PTR(len) : NULL, (GLint*)RT_PTR(size),
                    (GLenum*)RT_PTR(type), (GLchar*)RT_PTR(name));
}
void w2c_a_ed(struct w2c_a* e, u32 p, u32 i, u32 max, u32 len, u32 size, u32 type, u32 name) {
  glGetActiveUniform(p, i, max, len ? (GLsizei*)RT_PTR(len) : NULL, (GLint*)RT_PTR(size),
                     (GLenum*)RT_PTR(type), (GLchar*)RT_PTR(name));
}
void w2c_a_Nc(struct w2c_a* e, u32 p, u32 loc, u32 out) { glGetUniformfv(p, (GLint)loc, (GLfloat*)RT_PTR(out)); }
void w2c_a_Mc(struct w2c_a* e, u32 p, u32 loc, u32 out) { glGetUniformiv(p, (GLint)loc, (GLint*)RT_PTR(out)); }

/* ---------------- uniforms ---------------- */
#define LOC(l) ((GLint)(l))
void w2c_a_cc(struct w2c_a* e, u32 l, f32 a) { glUniform1f(LOC(l), a); }
void w2c_a_0x5Fb(struct w2c_a* e, u32 l, f32 a, f32 b) { glUniform2f(LOC(l), a, b); }
void w2c_a_Wb(struct w2c_a* e, u32 l, f32 a, f32 b, f32 c) { glUniform3f(LOC(l), a, b, c); }
void w2c_a_Sb(struct w2c_a* e, u32 l, f32 a, f32 b, f32 c, f32 d) { glUniform4f(LOC(l), a, b, c, d); }
void w2c_a_ac(struct w2c_a* e, u32 l, u32 a) { glUniform1i(LOC(l), (GLint)a); }
void w2c_a_Yb(struct w2c_a* e, u32 l, u32 a, u32 b) { glUniform2i(LOC(l), (GLint)a, (GLint)b); }
void w2c_a_Ub(struct w2c_a* e, u32 l, u32 a, u32 b, u32 c) { glUniform3i(LOC(l), (GLint)a, (GLint)b, (GLint)c); }
void w2c_a_Qb(struct w2c_a* e, u32 l, u32 a, u32 b, u32 c, u32 d) {
  glUniform4i(LOC(l), (GLint)a, (GLint)b, (GLint)c, (GLint)d);
}
void w2c_a_bc(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform1fv(LOC(l), n, (GLfloat*)RT_PTR(v)); }
void w2c_a_Zb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform2fv(LOC(l), n, (GLfloat*)RT_PTR(v)); }
void w2c_a_Vb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform3fv(LOC(l), n, (GLfloat*)RT_PTR(v)); }
void w2c_a_Rb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform4fv(LOC(l), n, (GLfloat*)RT_PTR(v)); }
void w2c_a_0x24b(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform1iv(LOC(l), n, (GLint*)RT_PTR(v)); }
void w2c_a_Xb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform2iv(LOC(l), n, (GLint*)RT_PTR(v)); }
void w2c_a_Tb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform3iv(LOC(l), n, (GLint*)RT_PTR(v)); }
void w2c_a_Pb(struct w2c_a* e, u32 l, u32 n, u32 v) { glUniform4iv(LOC(l), n, (GLint*)RT_PTR(v)); }
void w2c_a_Ob(struct w2c_a* e, u32 l, u32 n, u32 t, u32 v) { glUniformMatrix2fv(LOC(l), n, !!t, (GLfloat*)RT_PTR(v)); }
void w2c_a_Nb(struct w2c_a* e, u32 l, u32 n, u32 t, u32 v) { glUniformMatrix3fv(LOC(l), n, !!t, (GLfloat*)RT_PTR(v)); }
void w2c_a_Mb(struct w2c_a* e, u32 l, u32 n, u32 t, u32 v) { glUniformMatrix4fv(LOC(l), n, !!t, (GLfloat*)RT_PTR(v)); }

/* ---------------- vertex attributes & drawing ---------------- */
void w2c_a_qd(struct w2c_a* e, u32 i) { glEnableVertexAttribArray(i); }
void w2c_a_ud(struct w2c_a* e, u32 i) { glDisableVertexAttribArray(i); }
void w2c_a_Jb(struct w2c_a* e, u32 i, f32 x) { glVertexAttrib1f(i, x); }
void w2c_a_Hb(struct w2c_a* e, u32 i, f32 x, f32 y) { glVertexAttrib2f(i, x, y); }
void w2c_a_Fb(struct w2c_a* e, u32 i, f32 x, f32 y, f32 z) { glVertexAttrib3f(i, x, y, z); }
void w2c_a_Db(struct w2c_a* e, u32 i, f32 x, f32 y, f32 z, f32 w) { glVertexAttrib4f(i, x, y, z, w); }
void w2c_a_Ib(struct w2c_a* e, u32 i, u32 v) { glVertexAttrib1fv(i, (GLfloat*)RT_PTR(v)); }
void w2c_a_Gb(struct w2c_a* e, u32 i, u32 v) { glVertexAttrib2fv(i, (GLfloat*)RT_PTR(v)); }
void w2c_a_Eb(struct w2c_a* e, u32 i, u32 v) { glVertexAttrib3fv(i, (GLfloat*)RT_PTR(v)); }
void w2c_a_Cb(struct w2c_a* e, u32 i, u32 v) { glVertexAttrib4fv(i, (GLfloat*)RT_PTR(v)); }
void w2c_a_ie(struct w2c_a* e, u32 i, u32 d) { glVertexAttribDivisor(i, d); }
void w2c_a_Kc(struct w2c_a* e, u32 i, u32 p, u32 out) { glGetVertexAttribfv(i, p, (GLfloat*)RT_PTR(out)); }
void w2c_a_Jc(struct w2c_a* e, u32 i, u32 p, u32 out) { glGetVertexAttribiv(i, p, (GLint*)RT_PTR(out)); }
void w2c_a_Ic(struct w2c_a* e, u32 i, u32 p, u32 out) { RT_U32(out) = 0; }  // pointers can't cross into wasm

static GLint rt_bound(GLenum binding) {
  GLint b = 0;
  glGetIntegerv(binding, &b);
  return b;
}

// With a buffer bound, ptr is an offset; without one it is client memory in the wasm heap.
void w2c_a_Bb(struct w2c_a* e, u32 i, u32 size, u32 type, u32 norm, u32 stride, u32 ptr) {
  const void* p = rt_bound(GL_ARRAY_BUFFER_BINDING) ? (const void*)(uintptr_t)ptr : RT_PTR(ptr);
  glVertexAttribPointer(i, (GLint)size, type, !!norm, stride, p);
}

void w2c_a_td(struct w2c_a* e, u32 mode, u32 first, u32 count) { glDrawArrays(mode, (GLint)first, count); }
void w2c_a_ke(struct w2c_a* e, u32 mode, u32 first, u32 count, u32 inst) {
  glDrawArraysInstanced(mode, (GLint)first, count, inst);
}
void w2c_a_sd(struct w2c_a* e, u32 mode, u32 count, u32 type, u32 idx) {
  const void* p = rt_bound(GL_ELEMENT_ARRAY_BUFFER_BINDING) ? (const void*)(uintptr_t)idx : RT_PTR(idx);
  glDrawElements(mode, count, type, p);
}
void w2c_a_je(struct w2c_a* e, u32 mode, u32 count, u32 type, u32 idx, u32 inst) {
  const void* p = rt_bound(GL_ELEMENT_ARRAY_BUFFER_BINDING) ? (const void*)(uintptr_t)idx : RT_PTR(idx);
  glDrawElementsInstanced(mode, count, type, p, inst);
}

/* ---------------- queries ---------------- */
void w2c_a_qa(struct w2c_a* e, u32 t, u32 q) {}  // timer queries: not available on GLES
void w2c_a_pa(struct w2c_a* e, u32 t) {}
void w2c_a_oa(struct w2c_a* e, u32 q, u32 t) {}
void w2c_a_na(struct w2c_a* e, u32 t, u32 p, u32 out) { if (out) RT_I32(out) = 0; }
void w2c_a_ka(struct w2c_a* e, u32 q, u32 p, u32 out) { if (out) memset(RT_PTR(out), 0, 8); }
void w2c_a_ja(struct w2c_a* e, u32 q, u32 p, u32 out) { if (out) memset(RT_PTR(out), 0, 8); }
void w2c_a_ma(struct w2c_a* e, u32 q, u32 p, u32 out) { if (out) RT_I32(out) = 1; }
void w2c_a_la(struct w2c_a* e, u32 q, u32 p, u32 out) { if (out) RT_I32(out) = 1; }

// How many values glGet* writes for a pname, so we never overrun the wasm buffer.
static int rt_get_count(GLenum p) {
  switch (p) {
    case GL_VIEWPORT: case GL_SCISSOR_BOX: case GL_COLOR_WRITEMASK:
    case GL_COLOR_CLEAR_VALUE: case GL_BLEND_COLOR:
      return 4;
    case GL_DEPTH_RANGE: case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE: case GL_MAX_VIEWPORT_DIMS:
      return 2;
    case GL_COMPRESSED_TEXTURE_FORMATS: {
      GLint n = 0;
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
      return n;
    }
    default:
      return 1;
  }
}

void w2c_a_Yc(struct w2c_a* e, u32 p, u32 out) {  // glGetIntegerv
  if (!out) return;
  GLint tmp[64];
  int n = rt_get_count(p);
  if (n > 64) n = 64;
  glGetIntegerv(p, tmp);
  memcpy(RT_PTR(out), tmp, sizeof(GLint) * (size_t)n);
}
void w2c_a_0x5Fc(struct w2c_a* e, u32 p, u32 out) {  // glGetFloatv
  if (!out) return;
  GLfloat tmp[64];
  int n = rt_get_count(p);
  if (n > 64) n = 64;
  glGetFloatv(p, tmp);
  memcpy(RT_PTR(out), tmp, sizeof(GLfloat) * (size_t)n);
}
void w2c_a_Na(struct w2c_a* e, u32 p, u32 out) { w2c_a_0x5Fc(e, p, out); }
void w2c_a_bd(struct w2c_a* e, u32 p, u32 out) {  // glGetBooleanv
  if (!out) return;
  GLboolean tmp[64];
  int n = rt_get_count(p);
  if (n > 64) n = 64;
  glGetBooleanv(p, tmp);
  memcpy(RT_PTR(out), tmp, (size_t)n);
}

// glGetString returns a pointer, so each string is copied into the wasm heap once.
u32 w2c_a_Qc(struct w2c_a* e, u32 name) {
  static u32 cache[8];
  int slot;
  switch (name) {
    case GL_VENDOR: slot = 0; break;
    case GL_RENDERER: slot = 1; break;
    case GL_VERSION: slot = 2; break;
    case GL_SHADING_LANGUAGE_VERSION: slot = 3; break;
    case GL_EXTENSIONS: slot = 4; break;
    default: return 0;
  }
  if (!cache[slot]) {
    // The game was built against WebGL: present it the WebGL 1 strings it expects.
    const char* s;
    switch (name) {
      case GL_VERSION: s = "OpenGL ES 2.0 (WebGL 1.0)"; break;
      case GL_SHADING_LANGUAGE_VERSION: s = "OpenGL ES GLSL ES 1.00 (WebGL GLSL ES 1.0)"; break;
      default: s = (const char*)glGetString(name); break;
    }
    if (!s) s = "";
    u32 len = (u32)strlen(s) + 1;
    cache[slot] = rt_malloc(len);
    memcpy(RT_PTR(cache[slot]), s, len);
  }
  return cache[slot];
}
