
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#ifdef _WIN32
#include <memory>
#else
//#include <tr1/memory>
#endif

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif 

#include "../core/MLProperty.h"
#include "MLDSPOps.h"
#include "MLDSPFilters.h"
#include "MLDSPGens.h"
#include "MLDSPUtils.h"

