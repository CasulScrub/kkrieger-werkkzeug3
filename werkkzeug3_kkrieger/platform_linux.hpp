// Platform compatibility header for Linux build
// Replaces MSVC-specific keywords with Linux equivalents

#ifndef __PLATFORM_LINUX_HPP__
#define __PLATFORM_LINUX_HPP__

// Replace MSVC __forceinline with inline or always_inline
#define __forceinline inline __attribute__((always_inline))

// Linux default calling convention is cdecl, replace MSVC __cdecl
#define __cdecl

// Replace MSVC __declspec(align) with GCC attribute
#define __declspec(x) __attribute__((x))

// Special case: __attribute__((aligned(16)))
// Note: The macro above handles this correctly

// Replace  with nothing
#define 
#define 

// Replace __attribute__((noreturn)) with GCC equivalent
#define __attribute__((noreturn)) __attribute__((noreturn))

// Handle __int64 (already compatible on Linux/gcc)
// No replacement needed - __int64 works on GCC

#endif
