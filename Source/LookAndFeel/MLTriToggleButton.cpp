
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLTriToggleButton.h"
#include "MLLookAndFeel.h"

MLTriToggleButton::MLTriToggleButton() :
    MLButton()
{
    setAttribute("tri_button", true);
    setOpaque(false);
}

MLTriToggleButton::~MLTriToggleButton()
{
}

void MLTriToggleButton::setAttribute(MLSymbol attr, float val)
{
	static const MLSymbol valueSym("value");
	MLWidget::setAttribute(attr, val);
	
	if (attr == valueSym)
	{
		// update state without notify
        mState = (int)val;
	}
}

void MLTriToggleButton::paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	
	// colors
	const Colour offColor (findColour (MLLookAndFeel::darkFillColor));
	const Colour onColor (findColour (MLButton::buttonOnColourId));
	const Colour offBrightColor (offColor.getHue(), offColor.getSaturation(), jmin(offColor.getBrightness() + 0.1, 1.), offColor.getFloatAlpha());
	const Colour onBrightColor (onColor.getHue(), onColor.getSaturation(), jmin(onColor.getBrightness() + 0.1, 1.), onColor.getFloatAlpha());
	const Colour offOverColor ((mDoRollover && isMouseOverButton) ? offBrightColor : offColor);
	const Colour onOverColor ((mDoRollover && isMouseOverButton) ? onBrightColor : onColor);
	const Colour bc = (getToggleState() ? onOverColor : offOverColor);
	const float alpha = isEnabled() ? 1.f : 0.25f;
	const Colour textColor (findColour (MLButton::textColourId).withMultipliedAlpha (alpha));
	const Colour track_hard (findColour(MLLookAndFeel::outlineColor).withMultipliedAlpha (alpha));
	const Colour brightColor = Colour(bc.getHue(), bc.getSaturation(), jmin(bc.getBrightness() + 0.1, 1.), bc.getFloatAlpha());
	Colour buttonColor = bc.withAlpha (alpha);
	Colour outlineColor, outlineOnColor, outlineOffColor;
	outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(onOverColor.withMultipliedAlpha(0.625f));
	outlineOffColor = findColour(MLLookAndFeel::outlineColor);
	outlineColor = getToggleState() ? outlineOnColor : outlineOffColor;
	outlineColor = outlineColor.withAlpha (alpha);
    
	if (mImage.isValid())
	{
		buttonColor = buttonColor.overlaidWith(onColor.withMultipliedAlpha(0.25f));
	}
	
	// geometry
    const int width = getWidth();
    const int height = getHeight();
	int toggleSize = myLookAndFeel->getToggleButtonSize() * getWidgetGridUnitSize();
	int halfSize = toggleSize/2;
	int sixthSize = halfSize/3;
	int thirdSize = sixthSize*2 + 1;
    sixthSize = max(sixthSize, 1);
    thirdSize = max(thirdSize, 2);
    
	// get int center
	int cx = width/2 - 1;
	int cy = height/2 - 1;
	
	// get toggle rect
	int toggleX = cx - halfSize;
	int toggleY = cy - halfSize;
	int toggleWidth = halfSize*2;
	int toggleHeight = halfSize*2;
	
	int flair = 0;
	if (isButtonDown) flair |= (eMLAdornPressed);
	flair |= eMLAdornShadow;
	flair |= eMLAdornGlow;
	
	const float cornerSize = 0.;
    
    // TODO toggle buttons are even-sized! I don’t think this was the idea.
    // so there are some messy +1 and -1s going on here. investigate.

    // dark background
    myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight,
                                      cornerSize, offOverColor, outlineOffColor, kMLButtonOutlineThickness, flair, 0., 0.);        
    g.saveState();
    switch(mState)
    {
        case 0: // left
            g.reduceClipRegion(toggleX - 1, toggleY - 1, thirdSize + 1, toggleHeight + 1);
            break;
        case 1: // center
            g.reduceClipRegion(cx - sixthSize, toggleY - 1, thirdSize - 1, toggleHeight + 1);
            break;
        case 2: // right
            g.reduceClipRegion(toggleX + toggleWidth - thirdSize, toggleY - 1, thirdSize + 1, toggleHeight + 1);
           break;
        default:
            break;                
    }
     
    myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight,
                                      cornerSize, onOverColor, outlineOnColor, mLineThickness, flair, 0., 0.);
    g.restoreState();

}

void MLTriToggleButton::clicked (const ModifierKeys& modifiers)
{
    int state = getAttribute("value");
    state += 1;
    if(state > 2) state = 0;
    setAttribute("value", state);
    MLButton::clicked();
}

void MLTriToggleButton::resizeWidget(const MLRect& b, const int u)
{
	Component* pC = getComponent();
	mLineThickness = u/64.f;
	if(pC)
	{
		MLRect bb = b;
		bb.expand(-2);
		
		// make sure width is odd
		int newWidth = (bb.width()/2)*2 + 1;
        
		// adapt vrect to juce rect
		Rectangle<int> c(bb.left(), bb.top(), newWidth, bb.height());
		
		pC->setBounds(c);
	}
}

