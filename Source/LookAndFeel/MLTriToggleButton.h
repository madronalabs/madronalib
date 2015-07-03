
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_TRITOGGLE_BUTTON_HEADER__
#define __ML_TRITOGGLE_BUTTON_HEADER__

#include "MLUI.h"
#include "MLButton.h"

class MLTriToggleButton :
public MLButton
{
public:
    MLTriToggleButton();
    ~MLTriToggleButton();
	
	void paint(Graphics& g) override;
    void clicked () override;
    void resizeWidget(const MLRect& b, const int) override;
	float getLabelVerticalOffset() override { return 1.f; }
    
protected:
	float mLineThickness;
private:
 	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLTriToggleButton);
};

#endif   // __ML_TRITOGGLE_BUTTON_HEADER__
