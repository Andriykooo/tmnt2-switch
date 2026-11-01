// File syscalls. Emscripten's in-memory FS is replaced by the host file system:
// "/Common/..." (the preloaded package) and anything else the game writes live under
// rt_data_dir(); "/wfs/..." (the mounted game assets) lives under rt_asset_dir().
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rt.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

// musl/wasi errno values as seen by the wasm side.
#define WE_BADF 8
#define WE_EXIST 20
#define WE_INVAL 28
#define WE_NOENT 44
#define WE_NOTTY 59

// emscripten open() flags
#define WO_ACCMODE 3
#define WO_CREAT 0100
#define WO_EXCL 0200
#define WO_TRUNC 01000
#define WO_APPEND 02000
#define WO_DIRECTORY 0200000

#define RT_MAX_FD 64
static int rt_fds[RT_MAX_FD];  // wasm fd -> host fd, -1 when closed
static int rt_fds_init = 0;

static void rt_fs_init(void) {
  if (rt_fds_init) return;
  for (int i = 0; i < RT_MAX_FD; i++) rt_fds[i] = -1;
  rt_fds[0] = 0;
  rt_fds[1] = 1;
  rt_fds[2] = 2;
  rt_fds_init = 1;
}

static void rt_host_path(const char* wasm_path, char* out, size_t size) {
  size_t prefix;
  if (strncmp(wasm_path, "/wfs/", 5) == 0) {
    prefix = strlen(rt_asset_dir()) + 1;
    snprintf(out, size, "%s/%s", rt_asset_dir(), wasm_path + 5);
  } else {
    prefix = strlen(rt_data_dir());
    snprintf(out, size, "%s%s%s", rt_data_dir(), wasm_path[0] == '/' ? "" : "/", wasm_path);
  }
  // The save file is "/ifs/ifs:tmntsave.dat" (RenderWare keeps the device prefix);
  // ':' is fine in emscripten's MEMFS but not on FAT/NTFS.
  for (char* c = out + prefix; *c; c++)
    if (*c == ':') *c = '_';
}

// The web build keeps saves in an IDBFS mount; here that directory lives under the data dir.
void rt_fs_mkdir(const char* wasm_path) {
  char host[512];
  rt_host_path(wasm_path, host, sizeof host);
#ifdef _WIN32
  mkdir(host);
#else
  mkdir(host, 0777);
#endif
  RT_LOG("[fs] mount %s -> %s\n", wasm_path, host);
}

static int rt_errno_to_wasm(int e) {
  switch (e) {
    case ENOENT: return WE_NOENT;
    case EEXIST: return WE_EXIST;
    case EBADF: return WE_BADF;
    default: return WE_INVAL;
  }
}

static int rt_host_fd(u32 fd) {
  rt_fs_init();
  return fd < RT_MAX_FD ? rt_fds[fd] : -1;
}

// emscripten struct stat (wasm32), see Hb() in the glue
static void rt_write_stat(u32 buf, const struct stat* st) {
  memset(RT_PTR(buf), 0, 96);
  RT_U32(buf + 0) = (u32)st->st_dev;
  RT_U32(buf + 4) = S_ISDIR(st->st_mode) ? 0040755 : 0100644;
  RT_U32(buf + 8) = 1;
  int64_t size = st->st_size;
  memcpy(RT_PTR(buf + 24), &size, 8);
  RT_U32(buf + 32) = 4096;
  RT_U32(buf + 36) = (u32)((st->st_size + 511) / 512);
  int64_t t = st->st_mtime;
  memcpy(RT_PTR(buf + 40), &t, 8);
  memcpy(RT_PTR(buf + 56), &t, 8);
  memcpy(RT_PTR(buf + 72), &t, 8);
  int64_t ino = (int64_t)st->st_ino;
  memcpy(RT_PTR(buf + 88), &ino, 8);
}

static u32 rt_stat_path(u32 path, u32 buf) {
  char host[512];
  rt_host_path(RT_STR(path), host, sizeof host);
  struct stat st;
  if (stat(host, &st) != 0) return (u32)-WE_NOENT;
  rt_write_stat(buf, &st);
  return 0;
}

// pb: __syscall_openat(dirfd, path, flags, varargs)
u32 w2c_a_pb(struct w2c_a* env, u32 dirfd, u32 path, u32 flags, u32 varargs) {
  rt_fs_init();
  const char* p = RT_STR(path);
  char host[512];
  rt_host_path(p, host, sizeof host);

  if (flags & WO_DIRECTORY) {
    struct stat st;
    if (stat(host, &st) != 0 || !S_ISDIR(st.st_mode)) return (u32)-WE_NOENT;
  }

  int hflags = O_BINARY;
  switch (flags & WO_ACCMODE) {
    case 0: hflags |= O_RDONLY; break;
    case 1: hflags |= O_WRONLY; break;
    default: hflags |= O_RDWR; break;
  }
  if (flags & WO_CREAT) hflags |= O_CREAT;
  if (flags & WO_EXCL) hflags |= O_EXCL;
  if (flags & WO_TRUNC) hflags |= O_TRUNC;
  if (flags & WO_APPEND) hflags |= O_APPEND;

  int hfd = (flags & WO_DIRECTORY) ? open(host, O_RDONLY) : open(host, hflags, 0644);
  if (hfd < 0) {
    if (rt_verbose || (flags & WO_CREAT)) RT_LOG("[fs] open %s -> %s failed (errno %d)\n", p, host, errno);
    return (u32)-rt_errno_to_wasm(errno);
  }
  for (int i = 3; i < RT_MAX_FD; i++) {
    if (rt_fds[i] < 0) {
      rt_fds[i] = hfd;
      if (rt_verbose) RT_LOG("[fs] open %s -> fd %d\n", p, i);
      return (u32)i;
    }
  }
  close(hfd);
  return (u32)-WE_INVAL;
}

// ea: fd_close
u32 w2c_a_ea(struct w2c_a* env, u32 fd) {
  int hfd = rt_host_fd(fd);
  if (hfd < 0) return WE_BADF;
  if (fd > 2) {
    close(hfd);
    rt_fds[fd] = -1;
  }
  return 0;
}

// nb: fd_read(fd, iovs, iovs_len, nread_ptr)
u32 w2c_a_nb(struct w2c_a* env, u32 fd, u32 iovs, u32 count, u32 nread) {
  int hfd = rt_host_fd(fd);
  if (hfd < 0) return WE_BADF;
  u32 total = 0;
  for (u32 i = 0; i < count; i++) {
    u32 base = RT_U32(iovs + 8 * i), len = RT_U32(iovs + 8 * i + 4);
    int got = len ? (int)read(hfd, RT_PTR(base), len) : 0;
    if (got < 0) return rt_errno_to_wasm(errno);
    total += (u32)got;
    if ((u32)got < len) break;
  }
  RT_U32(nread) = total;
  return 0;
}

// mb: fd_write(fd, iovs, iovs_len, nwritten_ptr)
u32 w2c_a_mb(struct w2c_a* env, u32 fd, u32 iovs, u32 count, u32 nwritten) {
  int hfd = rt_host_fd(fd);
  if (hfd < 0) return WE_BADF;
  u32 total = 0;
  for (u32 i = 0; i < count; i++) {
    u32 base = RT_U32(iovs + 8 * i), len = RT_U32(iovs + 8 * i + 4);
    if (!len) continue;
    int out = (fd <= 2) ? (int)fwrite(RT_PTR(base), 1, len, stderr) : (int)write(hfd, RT_PTR(base), len);
    if (out < 0) return rt_errno_to_wasm(errno);
    total += (u32)out;
  }
  RT_U32(nwritten) = total;
  return 0;
}

// gb: fd_seek(fd, offset, whence, newoffset_ptr)
u32 w2c_a_gb(struct w2c_a* env, u32 fd, u64 offset, u32 whence, u32 out) {
  int hfd = rt_host_fd(fd);
  if (hfd < 0) return WE_BADF;
  int64_t pos = (int64_t)lseek(hfd, (off_t)(int64_t)offset, (int)whence);
  if (pos < 0) return rt_errno_to_wasm(errno);
  memcpy(RT_PTR(out), &pos, 8);
  return 0;
}

// qb: fd_sync
u32 w2c_a_qb(struct w2c_a* env, u32 fd) { return rt_host_fd(fd) < 0 ? WE_BADF : 0; }

// lb: __syscall_fstat64(fd, buf)
u32 w2c_a_lb(struct w2c_a* env, u32 fd, u32 buf) {
  int hfd = rt_host_fd(fd);
  if (hfd < 0) return (u32)-WE_BADF;
  struct stat st;
  if (fstat(hfd, &st) != 0) return (u32)-WE_BADF;
  rt_write_stat(buf, &st);
  return 0;
}

// jb: __syscall_stat64(path, buf) / kb: __syscall_lstat64(path, buf)
u32 w2c_a_jb(struct w2c_a* env, u32 path, u32 buf) { return rt_stat_path(path, buf); }
u32 w2c_a_kb(struct w2c_a* env, u32 path, u32 buf) { return rt_stat_path(path, buf); }

// ib: __syscall_newfstatat(dirfd, path, buf, flags)
u32 w2c_a_ib(struct w2c_a* env, u32 dirfd, u32 path, u32 buf, u32 flags) { return rt_stat_path(path, buf); }

// hb: __syscall_getcwd(buf, size)
u32 w2c_a_hb(struct w2c_a* env, u32 buf, u32 size) {
  if (size < 2) return (u32)-WE_INVAL;
  strcpy((char*)RT_PTR(buf), "/");
  return 2;
}

// ob: __syscall_ioctl - only terminal queries reach here
u32 w2c_a_ob(struct w2c_a* env, u32 fd, u32 op, u32 varargs) { return (u32)-WE_NOTTY; }

// da: __syscall_fcntl64
u32 w2c_a_da(struct w2c_a* env, u32 fd, u32 cmd, u32 varargs) { return 0; }

// vb: environ_get / wb: environ_sizes_get - empty environment
u32 w2c_a_vb(struct w2c_a* env, u32 env_ptrs, u32 buf) { return 0; }
u32 w2c_a_wb(struct w2c_a* env, u32 count, u32 size) {
  RT_U32(count) = 0;
  RT_U32(size) = 0;
  return 0;
}
