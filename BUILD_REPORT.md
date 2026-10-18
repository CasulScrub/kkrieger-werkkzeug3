PORT STATUS: kkrieger-werkkzeug3 Linux Build - Final Report

STEPS COMPLETED:
1. Clone: https://github.com/CasulScrub/kkrieger-werkkzeug3 cloned to /tmp/kkrieger-study
2. Windows Files Removed:
   - All win*.cpp, win*.hpp files (winpage, winview, winpara, winspline, wintimeline, wintool, wincalc, winnova, winnovaview)
   - Entire yasm-1.3.0/ directory (Windows assembler)
   - Windows .rc, .sln, .vcxproj, .vcproj files
   - v2/win32.inc and v2/libv2/win32.inc
3. Build Setup:
   - Created CMakeLists.txt
   - Created Makefile
4. Platform Fixes Applied:
   - Replaced __forceinline with inline + always_inline
   - Replaced __cdecl with nothing (Linux default cdecl)
   - Replaced __declspec(align(16)) with __attribute__((aligned(16)))
   - Replaced __declspec(dllexport/dllimport/noreturn) with equivalents
   - Created stub headers: winpara.hpp, winview.hpp, wintool.hpp, winpage.hpp, wintimeline.hpp, winspline.hpp, wincalc.hpp, winnova.hpp, winnovaview.hpp, windows.h, ole2.h
   - Fixed include paths (added -Imaterials, -Is., -Iwerkkzeug3_kkrieger)
   - Fixed operator new declarations (changed unsigned int to size_t)

REMAINING ISSUES (would require further work):
- __int64 references need checking
- #pragma intrinsic and #pragma inline_* directives still present (ignored by GCC but noisy)
- Some Windows-specific functionality (GUI dialogs, DirectX references) is missing
- The project is fundamentally Windows-only game code; full Linux compilation requires significant code changes
- Full build would need SDL/GL replacements for DirectX components

BUILD RESULT:
- Initial build shows compilation errors reduced from 20+ to a few remaining issues
- The code is now closer to Linux-compatible but still requires substantial work
- The main engine files (werkops, shadercompile, geneffect) can now be parsed by GCC

NOTES:
This is a research/study task - the repository is a Windows-only demo game (kkrieger)
with extensive use of Visual Studio project files, Windows API, and DirectX shader code.
Full Linux port would require replacing the rendering layer (DirectX -> OpenGL/Vulkan),
replacing Windows GUI elements, and adapting the shader compilation pipeline.
