
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLTextButton.h"
#include "MLLookAndFeel.h"

MLTextButton::MLTextButton()
    : MLButton ()
{
}

MLTextButton::~MLTextButton()
{
}

void MLTextButton::paint(Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->drawBackground(g, this);
	const Colour c (findColour (MLTextButton::buttonColourId));	
	const Colour t (findColour (MLTextButton::textColourId));	
    myLookAndFeel->drawButtonBackground(g, *this, c, mOver, mDown, mLineThickness);
	myLookAndFeel->drawButtonText(g, *this, t, mOver, mDown);
}

void MLTextButton::doPropertyChangeAction(MLSymbol property, const MLProperty& val)
{
	if (property == "text")
	{
		// this is needed because of MLMenuButton's file name stripping. see "strip" .
		// maybe not such a good design for that feature.
		setProperty("processed_text", val);
		repaint();
	}
	else
	{
		MLButton::doPropertyChangeAction(property, val);
	}
}

