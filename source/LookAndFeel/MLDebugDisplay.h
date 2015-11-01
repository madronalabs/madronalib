
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DEBUG_DISPLAY_HEADER__
#define __ML_DEBUG_DISPLAY_HEADER__

#include "JuceHeader.h"
#include "MLUI.h"
#include "MLWidget.h"
#include <sstream>

class MLDebugDisplay : 
	public Component,
	public MLWidget,
	public MLTextStreamListener,
	public Timer
{
public:
    MLDebugDisplay();
    ~MLDebugDisplay();

	void display();

protected:
	void resizeWidget(const MLRect& b, const int);
	void timerCallback();

private:
	std::unique_ptr<CodeDocument> mpDoc;
	std::unique_ptr<CodeEditorComponent> mpComp;

	juce::CriticalSection mStreamLock;

};


#endif  //__ML_DEBUG_DISPLAY_HEADER__