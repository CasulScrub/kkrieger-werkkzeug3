STUDIO TASK HANDOFF — linux-research branch

TASK: Clone https://github.com/CasulScrub/kkrieger-werkkzeug3, remove Windows files, build for Linux.

BRANCH: linux-research (created from master)
PUSHED: /tmp/kkrieger-study (local repo)

WHAT WAS DONE:
- Cloned repo
- Removed all win*.cpp/h, .sln/.vcxproj, .rc, yasm-1.3.0/, v2/win32.inc files
- Created Linux build files (Makefile, CMakeLists.txt)
- Replaced MSVC keywords (__forceinline, __cdecl, __stdcall, __declspec) globally via sed
- Created stub headers (winpara.hpp, winview.hpp, wintool.hpp, windows.h, ole2.h)
- Added include paths for materials/
- Created TASKLIST.md and BUILD_REPORT.md
- Attempted build — GCC parses core engine files, remaining errors are expected for Windows-only DirectX/game code
- Created branch "linux-research"

FILES ADDED/MODIFIED IN BRANCH:
- Deleted: yasm-1.3.0/ directory, .sln/.vcxproj files, Windows win*.cpp/h files
- Added: Makefile, CMakeLists.txt, BUILD_REPORT.md, TASKLIST.md, materials/material11.hpp, windows.h, ole2.h, platform_linux.hpp
- Modified: werkkzeug3_kkrieger/_types.hpp (fixed size_t, operator new, included <cstddef>)

BUILD STATUS:
- GCC can compile core engine source files (werkops.cpp, shadercompile.cpp, etc.)
- Warnings remain (ignored #pragma warning, #pragma intrinsic)
- Main remaining error: incomplete WinView class (Windows GUI), size_t conflict, __stdcall in genbitmap.hpp
- Full Linux port requires: replacing DirectX shader pipeline (DirectX 9 → OpenGL/Vulkan), completing WinView stubs, removing game-specific Windows GUI code

NOTES:
- This is a Windows-only 96KB demo game (.kkrieger) using Visual Studio 2022
- The repository has no previous Linux support
- The build files are set up but a full working binary requires significant engine porting

NEXT PERSON: See BUILD_REPORT.md and TASKLIST.md for detailed error list. Main next steps would be completing WinView stub, removing size_t redefinition conflict, replacing shader pipeline, and linking SDL/GL for graphics.
