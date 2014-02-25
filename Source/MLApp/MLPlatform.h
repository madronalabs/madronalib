
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_PLATFORM_H
#define _ML_PLATFORM_H

#if (defined (_WIN32) || defined (_WIN64))
  #define       ML_WINDOWS 1
#elif defined (LINUX) || defined (__linux__)
  #define     ML_LINUX 1
#elif (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
    #define     ML_IOS 1
#elif defined (__APPLE__)
    #define     ML_MAC 1
#else
    #define     ML_UNKNOWN 1 // this happens with Apple's Rez for example, so can't cause an error
#endif

#endif // _ML_PLATFORM_H