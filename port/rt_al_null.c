// Silent OpenAL for the PC harness. Buffers are "played" in real time so the game's
// streaming code sees them drain at the right pace, but no sound is produced.
#include "rt.h"

#define AL_MAX 256
#define AL_INITIAL 4113
#define AL_PLAYING 4114
#define AL_PAUSED 4115
#define AL_STOPPED 4116

typedef struct {
  double seconds;
} al_buffer;

typedef struct {
  int used;
  int state;
  u32 queue[64];
  int queued;
  double started_ms;  // when the head of the queue started playing
} al_source;

static al_buffer al_buffers[AL_MAX];
static al_source al_sources[AL_MAX];
static u32 al_next_id = 1;

static double al_bytes_per_frame(u32 format) {
  switch (format) {
    case 0x1100: return 1;   // MONO8
    case 0x1101: return 2;   // MONO16
    case 0x1102: return 2;   // STEREO8
    case 0x1103: return 4;   // STEREO16
    case 0x10010: return 4;  // MONO_FLOAT32
    case 0x10011: return 8;  // STEREO_FLOAT32
    default: return 4;
  }
}

// How many queued buffers have finished since the source started.
static int al_processed(al_source* s) {
  if (s->state != AL_PLAYING) return s->state == AL_STOPPED ? s->queued : 0;
  double elapsed = (rt_now_ms() - s->started_ms) / 1000.0;
  int done = 0;
  for (int i = 0; i < s->queued; i++) {
    u32 id = s->queue[i];
    double len = id < AL_MAX ? al_buffers[id].seconds : 0;
    if (elapsed < len) break;
    elapsed -= len;
    done++;
  }
  return done;
}

u32 w2c_a_ra(struct w2c_a* env, u32 name) { return 1; }                  // alcOpenDevice
u32 w2c_a_oe(struct w2c_a* env, u32 dev, u32 attrs) { return 1; }        // alcCreateContext
u32 w2c_a_n(struct w2c_a* env, u32 ctx) { return 1; }                    // alcMakeContextCurrent
void w2c_a_Vd(struct w2c_a* env, u32 ctx) {}                             // alcDestroyContext
u32 w2c_a_Kd(struct w2c_a* env, u32 dev) { return 1; }                   // alcCloseDevice
void w2c_a_Ca(struct w2c_a* env) {}                                      // resume AudioContext

void w2c_a_J(struct w2c_a* env, u32 n, u32 out) {                        // alGenBuffers
  for (u32 i = 0; i < n; i++) {
    u32 id = al_next_id++ % AL_MAX;
    al_buffers[id].seconds = 0;
    RT_U32(out + 4 * i) = id;
  }
}
void w2c_a_K(struct w2c_a* env, u32 n, u32 out) {                        // alGenSources
  for (u32 i = 0; i < n; i++) {
    u32 id = al_next_id++ % AL_MAX;
    memset(&al_sources[id], 0, sizeof al_sources[id]);
    al_sources[id].used = 1;
    al_sources[id].state = AL_INITIAL;
    RT_U32(out + 4 * i) = id;
  }
}
void w2c_a_fa(struct w2c_a* env, u32 n, u32 ids) {}                      // alDeleteBuffers
void w2c_a_ga(struct w2c_a* env, u32 n, u32 ids) {                       // alDeleteSources
  for (u32 i = 0; i < n; i++) {
    u32 id = RT_U32(ids + 4 * i);
    if (id < AL_MAX) al_sources[id].used = 0;
  }
}

void w2c_a_t(struct w2c_a* env, u32 buf, u32 format, u32 data, u32 size, u32 freq) {  // alBufferData
  if (buf < AL_MAX && freq) al_buffers[buf].seconds = size / al_bytes_per_frame(format) / (double)freq;
}

void w2c_a_r(struct w2c_a* env, u32 src) {                               // alSourcePlay
  if (src >= AL_MAX) return;
  if (al_sources[src].state != AL_PAUSED) al_sources[src].started_ms = rt_now_ms();
  al_sources[src].state = AL_PLAYING;
}
void w2c_a_D(struct w2c_a* env, u32 src) {                               // alSourcePause
  if (src < AL_MAX) al_sources[src].state = AL_PAUSED;
}
void w2c_a_u(struct w2c_a* env, u32 src) {                               // alSourceStop
  if (src < AL_MAX) al_sources[src].state = AL_STOPPED;
}

void w2c_a_H(struct w2c_a* env, u32 src, u32 n, u32 ids) {               // alSourceQueueBuffers
  if (src >= AL_MAX) return;
  al_source* s = &al_sources[src];
  for (u32 i = 0; i < n && s->queued < 64; i++) s->queue[s->queued++] = RT_U32(ids + 4 * i);
}
void w2c_a_I(struct w2c_a* env, u32 src, u32 n, u32 out) {               // alSourceUnqueueBuffers
  if (src >= AL_MAX) return;
  al_source* s = &al_sources[src];
  for (u32 i = 0; i < n && s->queued > 0; i++) {
    u32 id = s->queue[0];
    s->started_ms += (id < AL_MAX ? al_buffers[id].seconds : 0) * 1000.0;
    memmove(s->queue, s->queue + 1, sizeof(u32) * (size_t)(--s->queued));
    RT_U32(out + 4 * i) = id;
  }
}

void w2c_a_m(struct w2c_a* env, u32 src, u32 param, u32 out) {           // alGetSourcei
  if (src >= AL_MAX || !out) return;
  al_source* s = &al_sources[src];
  int v = 0;
  switch (param) {
    case 4112: v = s->state; break;          // SOURCE_STATE
    case 4117: v = s->queued; break;         // BUFFERS_QUEUED
    case 4118: v = al_processed(s); break;   // BUFFERS_PROCESSED
  }
  RT_I32(out) = v;
}
void w2c_a_E(struct w2c_a* env, u32 src, u32 param, u32 out) {           // alGetSourcef
  if (out) RT_F32(out) = 0;
}
void w2c_a_h(struct w2c_a* env, u32 src, u32 param, f32 v) {}            // alSourcef
void w2c_a_p(struct w2c_a* env, u32 src, u32 param, u32 v) {}            // alSourcei
void w2c_a_L(struct w2c_a* env, u32 src, u32 param, f32 x, f32 y, f32 z) {}  // alSource3f
