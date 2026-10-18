TASK LIST: Port kkrieger-werkkzeug3 from Windows to Linux - Task List

CURRENT STATE:
- Repository cloned to /tmp/kkrieger-study
- Windows-specific files removed
- Created CMakeLists.txt and Makefile
- Build fails with numerous platform-specific errors
- Created winpara.hpp stub, moved material11.hpp to root

ISSUES TO FIX (in order):
1. Include paths (add -Imaterials/ to compiler flags)
2. Replace __forceinline → inline in all .cpp/.hpp files
3. Replace __cdecl → remove (Linux default) in function declarations
4. Fix operator new/delete with __cdecl declarations
5. Fix wsprintfA references → replace with snprintf
6. Fix __declspec(align(16)) → __attribute__((aligned(16)))
7. Fix __declspec(dllexport/dllimport) → empty strings
8. Remove Windows pragmas (#pragma warning, #pragma intrinsic, etc.)
9. Check genoverlay.hpp includes need materials/ on include path
10. Create a proper build that compiles on Linux
11. Create stub headers for any remaining Windows-specific dependencies
