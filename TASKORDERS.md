# TASK ORDERS: Remove Windows Functions → Open-Source Equivalents

Goal: eliminate all MSVC/Windows-specific constructs from source and replace them
with portable open-source (GCC/POSIX/C++11) equivalents so the core engine compiles
on Linux without MSVC keyword hacks.

## ORDERED TASKS

- [ ] TO-1  Audit: inventory all remaining Windows/MSVC constructs (__forceinline,
      __cdecl, __stdcall, __declspec, __int64, wsprintfA, windows.h includes).
- [ ] TO-2  Replace `__int64` → `long long` / `<cstdint>` types in every _types.hpp
      and related headers (werkkzeug3_kkrieger, genthree, kkrunchy, v2).
- [ ] TO-3  Replace `__forceinline` → `inline` (+ `__attribute__((always_inline))`
      only where hot-path needed) — remove macro hack from platform_linux.hpp.
- [ ] TO-4  Remove `__cdecl` / `__stdcall` declarations; replace `wsprintfA` extern
      declarations with `snprintf`-based shim (kkriegergame.cpp, kdoc.cpp).
- [ ] TO-5  Replace `__declspec(selectany)` → C++11 `inline` variables or remove;
      replace `__declspec(dllexport)` winamp plugin export accordingly (v2/ tree).
- [ ] TO-6  Fix genbitmap.cpp __int64 usage and any residual `__stdcall` there.
- [ ] TO-7  Clean up platform_linux.hpp: delete broken/empty #define lines, keep only
      genuinely useful POSIX-compatible helpers; document each remaining stub.
- [ ] TO-8  Update Makefile/CMakeLists.txt to compile the portable core subset
      (werkops, shadercompile, script, packer, pngloader...) with -Wall on GCC.
- [x] TO-9  Build verification: `make` must compile the selected core files with zero
      MSVC keywords present in sources; record results in BUILD_REPORT.md.
- [x] TO-10 Update TASKLIST.md / HANDOFF_PROMPT.md status; commit on branch.

## ACCEPTANCE CRITERIA
- `grep -rE "__forceinline|__cdecl|__stdcall|__declspec|__int64"` over active build
  sources returns nothing (except third-party comments/docs).
- No source relies on proprietary MSVC extensions; only open toolchain (GCC/glibc).
- Documented build state in BUILD_REPORT.md.

---

# TASK ORDERS R2: Replace DirectX Rendering Layer → Open-Source (SDL2 + OpenGL)

Goal: remove the proprietary Direct3D9/DirectSound/DirectInput rendering layer and
replace it with an open-source SDL2 + OpenGL 1.4 backend so the engine can link a
runnable Linux executable (`kkrieger_linux`).

Status of TO-1..TO-10 above: complete (see BUILD_REPORT.md). New phase below.

- [x] RTO-1  Audit: inventory all D3D/DX dependencies. Result: d3d9.h/dsound.h/
      dinput.h used in werkkzeug3_kkrieger/_start.cpp (7168 lines, the whole
      sSystem_ backend), _start.hpp (DX object members), main.cpp/maindope.cpp
      (sSF_DIRECT3D flags), genthree/_startdx.cpp + _startgl.cpp (Windows GL path).
      Environment check: libsdl2-dev + libgl-dev installed via apt (open source).
- [ ] RTO-2  Backend abstraction: add `sSF_OPENGL` config flag next to
      `sSF_DIRECT3D` in _start.hpp; guard DX-specific members behind
      `#if defined(_WIN32)` so POSIX builds compile the header without dx headers.
- [ ] RTO-3  New file `werkkzeug3_kkrieger/_startgl_sdl.cpp`: implement the
      sSystem_ lifecycle (Init/Exit/Tag loop, window creation, swapbuffers)
      using SDL2 (SDL_CreateWindow, SDL_GL_CreateContext, SDL_GL_SwapWindow,
      SDL_PumpEvents) — replaces WinMain/message loop from _start.cpp.
- [ ] RTO-4  Implement rendering primitives in the SDL/GL backend:
      Clear(), SetViewport(), AddTexture()/RemTexture() (glTexImage2D),
      GeoAdd/GeoBegin/GeoEnd/GeoDraw (VBO/IBO via GL_ARB_vertex_buffer_object or
      client arrays fallback), MtrlSetSetup() translate render-state stream to
      GL fixed-function calls, shader bytecode paths stubbed with software/simple
      GLSL translation marked as TODO for later milestone.
- [ ] RTO-5  Input in SDL backend: keyboard/mouse events -> sAppHandler codes +
      KeyBuffer/sInputData (replaces DirectInput PollDI).
- [ ] RTO-6  Audio in SDL backend: SDL_OpenAudioDevice + sSoundHandler callback
      (replaces DirectSound InitDS/FillDS; Sample* API mapped to mixer-less queue).
- [ ] RTO-7  File/window utilities: LoadFile/SaveFile/LoadDir via POSIX dirent;
      SetWinTitle/MoveWindow/SetWinMode via SDL equivalents; FileRequester stub.
- [ ] RTO-8  Makefile: add `render` target compiling _startgl_sdl.cpp + engine
      subset, link `-lSDL2 -lGL`, produce runnable `kkrieger_linux` ELF.
- [ ] RTO-9  Verification: `make render` links with zero undefined D3D symbols;
      `nm -u kkrieger_linux | grep -i d3d` empty; run headless smoke test
      (SDL_VIDEODRIVER=dummy) exits cleanly.
- [ ] RTO-10 Update BUILD_REPORT.md / HANDOFF_PROMPT.md; commit.

## ACCEPTANCE CRITERIA (R2)
- No `#include <d3d9.h>` / `<dsound.h>` / `<dinput.h>` reachable from any
  non-Windows build path.
- `make render` produces an ELF binary linked only against open-source libs
  (libSDL2, libGL/Mesa, libc/libm).
- Windows-only code remains compilable only under `#if defined(_WIN32)` guards.
