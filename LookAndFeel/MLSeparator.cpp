
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSeparator.h"
#include "MLLookAndFeel.h"
//#include "../lookandfeel/juce_LookAndFeel.h"


//==============================================================================
MLSeparator::MLSeparator ()
{
 	setColour (foregroundColourId, findColour(MLLookAndFeel::markColor));
	setInterceptsMouseClicks(false, false);
}

MLSeparator::~MLSeparator()
{
}

void MLSeparator::setText (const String& text)
{
    displayedMessage = text;
}

void MLSeparator::lookAndFeelChanged()
{
	setOpaque (false);
}

void MLSeparator::colourChanged()
{
    lookAndFeelChanged();
}

void MLSeparator::paint (Graphics& g)
{
	const Colour fc = findColour(foregroundColourId);	
	int w = getWidth();
	int h = getHeight();
	const bool vertical = w < h;
	Path p;
	
	g.setColour (fc); 
	if (vertical)
	{
		g.drawVerticalLine(w/2, 0, h);
	}
	else
	{
		g.drawHorizontalLine(h/2, 0, w);
	}
}

void MLSeparator::visibilityChanged()
{

}
