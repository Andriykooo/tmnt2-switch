#!/bin/bash
# Builds tmnt2.nro. Run from the devkitPro MSYS2 shell:
#   C:\devkitPro\msys2\usr\bin\bash.exe -lc "cd <path to this repo> && ./build-switch.sh"
# gen/tmnt2.switch.o (the recompiled game, ~50 min) is only rebuilt when gen/tmnt2.c changes.
set -e

DKP=/opt/devkitpro
export PATH=$DKP/devkitA64/bin:$DKP/tools/bin:$PATH

ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"
# No mmap/guard pages on Horizon: wasm memory is malloc'd and every access is bounds-checked.
DEFS="-D__SWITCH__ -DWASM_RT_MAX_CALL_STACK_DEPTH=10000 -DWASM_RT_USE_MMAP=0 -include port/compat/switch_compat.h"
RT=tools/wabt-1.0.42/share/wabt/wasm2c
INC="-I$DKP/libnx/include -I$DKP/portlibs/switch/include -Iport/compat -Itools/inc -Itools/wabt-1.0.42/include -I$RT -Igen -Iport"
CFLAGS="-O2 -g -w -ffunction-sections $ARCH $DEFS $INC"

mkdir -p build/switch

# ~50 min with bounds checks; delete gen/tmnt2.switch.o to force it after changing DEFS.
if [ ! -f gen/tmnt2.switch.o ] || [ gen/tmnt2.c -nt gen/tmnt2.switch.o ]; then
  echo "== compiling recompiled game (slow)"
  aarch64-none-elf-gcc $CFLAGS -c gen/tmnt2.c -o gen/tmnt2.switch.o
fi

SRCS="port/switch_main.c port/rt_core.c port/rt_fs.c port/rt_display.c port/rt_gl.c port/rt_input.c
      port/rt_al_openal.c port/imports_gen.c $RT/wasm-rt-impl.c $RT/wasm-rt-mem-impl.c $RT/wasm-rt-exceptions-impl.c"
OBJS=""
for src in $SRCS; do
  obj=build/switch/$(basename "${src%.c}").o
  echo "== $src"
  aarch64-none-elf-gcc $CFLAGS -c "$src" -o "$obj"
  OBJS="$OBJS $obj"
done

echo "== linking"
# Mesa is C++, so link with g++ to pull in libstdc++.
aarch64-none-elf-g++ -specs=$DKP/libnx/switch.specs $ARCH -g -Wl,-Map,build/switch/tmnt2.map \
  -o build/switch/tmnt2.elf $OBJS gen/tmnt2.switch.o \
  -L$DKP/portlibs/switch/lib -L$DKP/libnx/lib \
  -lopenal -lSDL2 -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lm

nacptool --create "TMNT 2: Battle Nexus" "Konami / wasm2c port" "0.1.0" build/switch/tmnt2.nacp
# build/icon/icon.jpg: 256x256 JPEG cropped from the title screen.
elf2nro build/switch/tmnt2.elf build/switch/tmnt2.nro --nacp=build/switch/tmnt2.nacp --icon=build/icon/icon.jpg
ls -la build/switch/tmnt2.nro
