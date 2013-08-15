
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_AUDIOPROCESSORLISTENER_HEADER__
#define __ML_AUDIOPROCESSORLISTENER_HEADER__

#include "MLUI.h"

// our AU wrapper is a member of this class.  The interface is used to load and save AU preset files.

class MLAudioProcessorListener
{
public:
	enum msgType
	{
		kLoad = 0,
		kSave = 1,
		kLoadScale = 2,
		kLoadDefaultScale = 3
	};

    virtual ~MLAudioProcessorListener() {}

	virtual void loadFile(const File& f) = 0;
	virtual void saveToFile(const File& f) = 0;
};

#endif   // __ML_AUDIOPROCESSORLISTENER_HEADER__
