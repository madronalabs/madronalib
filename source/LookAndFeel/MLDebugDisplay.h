
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DEBUG_DISPLAY_HEADER__
#define __ML_DEBUG_DISPLAY_HEADER__

#include "MLTextStreamListener.h"
#include "JuceHeader.h"
#include "MLUI.h"
#include "MLWidget.h"
#include "MLTimer.h"
#include <sstream>

class MLDebugDisplay : 
	public Component,
	public MLWidget,
	public MLTextStreamListener
{
public:
    MLDebugDisplay(MLWidget* pContainer);
    ~MLDebugDisplay();

	void display();

protected:
	void resizeWidget(const MLRect& b, const int);

private:
	std::unique_ptr<CodeDocument> mpDoc;
	std::unique_ptr<CodeEditorComponent> mpComp;

	juce::CriticalSection mStreamLock;
	ml::Timer mTimer;
};


#endif  //__ML_DEBUG_DISPLAY_HEADER__
