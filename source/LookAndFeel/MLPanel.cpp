
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPanel.h"
#include "MLLookAndFeel.h"
#include "MLAppView.h"

//==============================================================================
MLPanel::MLPanel (MLWidget* pContainer) :
	MLWidget(pContainer)
{
 	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = (&(getRootViewResources(this).mLookAndFeel));
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	setColour (backgroundColourId, findColour(MLLookAndFeel::backgroundColor));
 	setColour (foregroundColourId, findColour(MLLookAndFeel::markColor));
	setInterceptsMouseClicks(false, false);
}

void MLPanel::setBackgroundColor(const Colour& color)
{
 	setColour (backgroundColourId, color);
}

MLPanel::~MLPanel()
{
}

void MLPanel::paint (Graphics&)
{
//	MLLookAndFeel* myLookAndFeel = (&(getRootViewResources(this).mLookAndFeel));
//	myLookAndFeel->drawBackground(g, this);

//	const Colour fc = findColour(foregroundColourId);	
//	g.setColour (fc); 
//	g.drawVerticalLine(0, 0, getHeight());
}

void MLPanel::resizeWidget(const MLRect& b, const int)
{
	Component* pC = getComponent();
	if(pC)
	{	
		// adapt vrect to juce rect
		Rectangle<int> c(b.left(), b.top(), b.width(), b.height());
		pC->setBounds(c);
	}
}


