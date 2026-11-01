// OpenAL imports mapped 1:1 onto openal-soft (Switch). Device and context handles are
// kept host-side; the game only sees small non-zero ids.
#include <AL/al.h>
#include <AL/alc.h>

#include "rt.h"

static ALCdevice* g_device;
static ALCcontext* g_context;

u32 w2c_a_ra(struct w2c_a* env, u32 name) {  // alcOpenDevice
  if (!g_device) g_device = alcOpenDevice(NULL);
  if (!g_device) RT_LOG("[al] alcOpenDevice failed\n");
  return g_device ? 1 : 0;
}
u32 w2c_a_oe(struct w2c_a* env, u32 dev, u32 attrs) {  // alcCreateContext
  if (!g_context && g_device) g_context = alcCreateContext(g_device, NULL);
  return g_context ? 1 : 0;
}
u32 w2c_a_n(struct w2c_a* env, u32 ctx) {  // alcMakeContextCurrent
  return alcMakeContextCurrent(ctx ? g_context : NULL) ? 1 : 0;
}
void w2c_a_Vd(struct w2c_a* env, u32 ctx) {  // alcDestroyContext
  if (g_context) alcDestroyContext(g_context);
  g_context = NULL;
}
u32 w2c_a_Kd(struct w2c_a* env, u32 dev) {  // alcCloseDevice
  if (g_device) alcCloseDevice(g_device);
  g_device = NULL;
  return 1;
}
void w2c_a_Ca(struct w2c_a* env) {}  // resume AudioContext

void w2c_a_J(struct w2c_a* env, u32 n, u32 out) { alGenBuffers(n, (ALuint*)RT_PTR(out)); }
void w2c_a_K(struct w2c_a* env, u32 n, u32 out) { alGenSources(n, (ALuint*)RT_PTR(out)); }
void w2c_a_fa(struct w2c_a* env, u32 n, u32 ids) { alDeleteBuffers(n, (ALuint*)RT_PTR(ids)); }
void w2c_a_ga(struct w2c_a* env, u32 n, u32 ids) { alDeleteSources(n, (ALuint*)RT_PTR(ids)); }
// The game streams voice/music even when voice*.bin / strbgm.vas are missing: the stream
// header is then read from uninitialised memory (SdStrAL.cpp, SdStrReadHeadWait), so
// format, rate and range are garbage and may run past the end of wasm memory (a data
// abort natively). Such buffers are submitted empty, which plays as silence.
static int rt_al_buffer_valid(u32 fmt, u32 data, u32 size, u32 freq) {
  if (fmt != AL_FORMAT_MONO8 && fmt != AL_FORMAT_MONO16 &&
      fmt != AL_FORMAT_STEREO8 && fmt != AL_FORMAT_STEREO16)
    return 0;
  if (freq < 4000 || freq > 96000) return 0;
  return (uint64_t)data + size <= RT_MEMORY->size;
}

void w2c_a_t(struct w2c_a* env, u32 buf, u32 fmt, u32 data, u32 size, u32 freq) {
  if (!rt_al_buffer_valid(fmt, data, size, freq)) {
    static int logged = 0;
    if (logged++ < 20)
      RT_LOG("[al] alBufferData(%u) dropped: fmt=0x%x data=%u size=%u freq=%u\n", buf, fmt, data, size, freq);
    static const short silence[2] = {0, 0};
    alBufferData(buf, AL_FORMAT_MONO16, silence, sizeof silence, 22050);
    return;
  }
  alBufferData(buf, fmt, RT_PTR(data), size, freq);
}
void w2c_a_r(struct w2c_a* env, u32 src) { alSourcePlay(src); }
void w2c_a_D(struct w2c_a* env, u32 src) { alSourcePause(src); }
void w2c_a_u(struct w2c_a* env, u32 src) { alSourceStop(src); }
void w2c_a_H(struct w2c_a* env, u32 src, u32 n, u32 ids) { alSourceQueueBuffers(src, n, (ALuint*)RT_PTR(ids)); }
void w2c_a_I(struct w2c_a* env, u32 src, u32 n, u32 ids) { alSourceUnqueueBuffers(src, n, (ALuint*)RT_PTR(ids)); }
void w2c_a_m(struct w2c_a* env, u32 src, u32 p, u32 out) { if (out) alGetSourcei(src, p, (ALint*)RT_PTR(out)); }
void w2c_a_E(struct w2c_a* env, u32 src, u32 p, u32 out) { if (out) alGetSourcef(src, p, (ALfloat*)RT_PTR(out)); }
void w2c_a_h(struct w2c_a* env, u32 src, u32 p, f32 v) { alSourcef(src, p, v); }
void w2c_a_p(struct w2c_a* env, u32 src, u32 p, u32 v) { alSourcei(src, p, (ALint)v); }
void w2c_a_L(struct w2c_a* env, u32 src, u32 p, f32 x, f32 y, f32 z) { alSource3f(src, p, x, y, z); }
