
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDrawableButton.h"
#include "MLLookAndFeel.h"


MLDrawableButton::MLDrawableButton ()
    : MLButton ()
{
	backgroundOff = Colours::transparentBlack;
	backgroundOn = findColour(MLLookAndFeel::backgroundColor);
}

MLDrawableButton::~MLDrawableButton()
{
}

//==============================================================================
void MLDrawableButton::setImage (const Drawable* img)
{
    jassert (img != 0); 
    if (img)        
	{
		normalImage = img->createCopy();
	}
}

void MLDrawableButton::paint (Graphics& g)
{ 
 	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
    Rectangle<int> imageSpace = getLocalBounds();
	if (isOpaque()) myLookAndFeel->drawBackground(g, this);
	const Colour c (findColour (TextButton::buttonColourId));

	float u = myLookAndFeel->getGridUnitSize();
	if (mDown)
	{
		imageSpace.translate(0, u/32.);
	}			  
	
	normalImage->setTransformToFit (imageSpace.toFloat(), RectanglePlacement::centred);
	normalImage->drawAt(g, 0, 0, 1.);
}

//==============================================================================

void MLDrawableButton::resizeWidget(const MLRect& b, const int u)
{
    MLButton::resizeWidget(b, u);
	Component* pC = getComponent();
	if(pC)
	{
		// adapt vrect to juce rect
		Rectangle<int> c(b.left(), b.top(), b.width(), b.height());

		pC->setBounds(c);
	}
}

