// port_shims.cpp — Open-source (POSIX) replacements for Windows API functions
// used by the kkrieger/werkkzeug sources. Replaces:
//   - OutputDebugStringA (Windows debug console output) -> stderr
// This file provides the extern "C" symbols declared across _types.cpp,
// genoverlay.cpp, kdoc.cpp etc., so no proprietary Windows code is required.

#include <cstdio>

extern "C" void OutputDebugStringA(const char *str)
{
  if (str) {
    fputs(str, stderr);
    fputc('\n', stderr);
  }
}

#include <cstdlib>
#include <new>

// Portable placement array-new matching the debug 'new' macro in _types.hpp.
void *operator new[](size_t size, const char *, int)
{
  void *p = ::malloc(size ? size : 1);
  return p;
}
