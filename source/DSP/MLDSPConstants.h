// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/
//
// MLDSPConstants.h

#pragma once

#ifdef _WIN32
// constexpr float FLT_MAX = 3.40282346638528860e+38f;
#endif

namespace ml
{				
	constexpr float kTwoPi = 6.2831853071795864769252867f;
	constexpr float kPi = 3.1415926535897932384626433f;
	constexpr float kOneOverTwoPi = 1.0f / kTwoPi;
	constexpr float kE = 2.718281828459045f;
	constexpr float kTwelfthRootOfTwo = 1.05946309436f;
	constexpr float kMinGain = 0.00001f; // 10e-5 = -120dB
} // namespace ml
