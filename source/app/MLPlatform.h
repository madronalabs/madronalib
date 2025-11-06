// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_PLATFORM_H
#define _ML_PLATFORM_H

#if (defined(_WIN32) || defined(_WIN64))
#define ML_WINDOWS 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

// WORKAROUND: MSVC Standard Library Bug - Missing _beginthreadex Declaration
//
// Issue: In certain MSVC versions/configurations, std::thread constructor fails with:
// - error C3861: '_beginthreadex': identifier not found
// - '_beginthreadex': is not a member of '`global namespace''
//
// Root Cause: MSVC's <thread> implementation internally uses _beginthreadex() but
// in some cases the compiler cannot find the function declaration, even when
// <process.h> is included. This appears to be related to:
// 1. Include order issues between Windows headers and standard library headers
// 2. Conditional compilation in MSVC headers that may skip the declaration
// 3. Runtime library linking configuration mismatches
//
// This explicit extern "C" declaration ensures _beginthreadex is visible to
// the std::thread implementation regardless of header include issues.

extern "C"
{
  uintptr_t __cdecl _beginthreadex(void* security, unsigned stack_size,
                                   unsigned(__stdcall* start_address)(void*), void* arglist,
                                   unsigned initflag, unsigned* thrdaddr);
}

#include <thread>

#elif defined(LINUX) || defined(__linux__)
#define ML_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define ML_IOS 1
#else
#define ML_MAC 1
#endif
#else
#define ML_UNKNOWN 1  // this happens with Apple's Rez for example, so can't cause an error
#endif

#endif  // _ML_PLATFORM_H
