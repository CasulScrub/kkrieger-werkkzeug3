// Platform compatibility header for Linux / open-source builds.
//
// NOTE: All MSVC-specific keywords (__forceinline, __cdecl, _,
// __declspec, __int64) have been REMOVED from the sources themselves and
// replaced with portable C++11 / GCC equivalents directly in the code.
// This header now only provides genuinely portable helpers; it no longer
// redefines reserved compiler keywords (which was fragile and non-standard).

#ifndef __PLATFORM_LINUX_HPP__
#define __PLATFORM_LINUX_HPP__

#include <cstddef>

// Portable aligned-storage helper (replacement for __declspec(align(16)) usage)
#define PORT_ALIGN16 __attribute__((aligned(16)))

// Portable forced-inline helper (replacement for MSVC __forceinline)
#define PORT_FORCEINLINE inline __attribute__((always_inline))

// Portable selectany replacement marker (was __declspec(selectany));
// single-definition semantics are handled by C++11 inline variables where used.
#define PORT_SELECTANY /* nothing: rely on inline/unique definitions */

#endif
