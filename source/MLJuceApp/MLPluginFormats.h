
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINFORMATS_H__
#define __ML_PLUGINFORMATS_H__

namespace MLPluginFormats
{

enum pluginFormat
{
	eUndefined = -1,
	eVSTPlugin = 0,
	eAUPlugin = 1,
	eStandalone = 2,
	eRTASPlugin = 3
};

}
#endif  // __ML_PLUGINFORMATS_H__
