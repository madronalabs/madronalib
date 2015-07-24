
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_TIME_H__
#define __ML_TIME_H__

#if WINDOWS
typedef   signed __int64                int64;
typedef unsigned __int64                uint64;
#else
typedef   signed long long              int64;
typedef unsigned long long              uint64;
#endif

int64 getMicroseconds();


#endif // __ML_TIME_H__
