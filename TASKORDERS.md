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
- [ ] TO-9  Build verification: `make` must compile the selected core files with zero
      MSVC keywords present in sources; record results in BUILD_REPORT.md.
- [ ] TO-10 Update TASKLIST.md / HANDOFF_PROMPT.md status; commit on branch.

## ACCEPTANCE CRITERIA
- `grep -rE "__forceinline|__cdecl|__stdcall|__declspec|__int64"` over active build
  sources returns nothing (except third-party comments/docs).
- No source relies on proprietary MSVC extensions; only open toolchain (GCC/glibc).
- Documented build state in BUILD_REPORT.md.
