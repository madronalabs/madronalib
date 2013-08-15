
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
    enum ButtonStyle
    {
        ImageFitted,
		ImageOnButtonBackground 
    };

	MLDrawableButton (const String& buttonName, ButtonStyle buttonStyle);
    MLDrawableButton();
    ~MLDrawableButton();

	void setAttribute(MLSymbol attr, float val);
	
	void setImage (const Drawable* img);

    void setButtonStyle (ButtonStyle newStyle);

    void setBackgroundColours (const Colour& toggledOffColour,
                               const Colour& toggledOnColour);

	// set an optional color to be used for background when hovering.
	void setHoverColor(const Colour& h);

    const Colour& getBackgroundColour() const throw();

	void resizeWidget(const MLRect& b, const int);

protected:
 
	void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown);
 
private:
    //==============================================================================
    ButtonStyle style;
    ScopedPointer <Drawable> normalImage;
    Colour backgroundOff, backgroundOn, backgroundHover, mGlowColor;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLDrawableButton);
};

#endif   // __ML_DRWBL_BUTTON_HEADER__
