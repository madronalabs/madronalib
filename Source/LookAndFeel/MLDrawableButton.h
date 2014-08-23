
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DRWBL_BUTTON_HEADER__
#define __ML_DRWBL_BUTTON_HEADER__

#include "MLUI.h"
#include "MLButton.h"

class  MLDrawableButton : 
	public MLButton
{
public:

    MLDrawableButton();
    ~MLDrawableButton();

	void paint (Graphics& g);
 	void setImage (const Drawable* img);
	void resizeWidget(const MLRect& b, const int);

private:
    //==============================================================================
    ScopedPointer <Drawable> normalImage;
    Colour backgroundOff, backgroundOn, backgroundHover, mGlowColor;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLDrawableButton);
};

#endif   // __ML_DRWBL_BUTTON_HEADER__
