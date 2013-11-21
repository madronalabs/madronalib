
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLTextButton.h"
#include "MLLookAndFeel.h"

MLTextButton::MLTextButton (const String& name, const String& )
    : MLButton (name)
{
  //  setTooltip (toolTip);
}

MLTextButton::~MLTextButton()
{
}

void MLTextButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->drawBackground(g, this);
	const Colour c (findColour (MLTextButton::buttonColourId));	
	const Colour t (findColour (MLTextButton::textColourId));	

    myLookAndFeel->drawButtonBackground (g, *this,
		c,
		isMouseOverButton,
		isButtonDown, mLineThickness);
	
    myLookAndFeel->drawButtonText (g, *this,
		t,
		isMouseOverButton,
		isButtonDown);
}

void MLTextButton::colourChanged()
{
    repaint();
}
