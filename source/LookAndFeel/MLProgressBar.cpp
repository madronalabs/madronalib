
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProgressBar.h"
#include "MLLookAndFeel.h"

//==============================================================================
MLProgressBar::MLProgressBar ()
{
 	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());
	setInterceptsMouseClicks(false, false);
}

MLProgressBar::~MLProgressBar()
{
}

void MLProgressBar::paint (Graphics& g)
{
	const Colour fc = findColour(MLLookAndFeel::labelColor);	
	const float progress = getFloatProperty("progress");

	Path gbounds;
	const Rectangle<int> & boundsRect (getLocalBounds());	
	gbounds.addRectangle(boundsRect);
	MLRange xRange(0., 1., boundsRect.getX(), boundsRect.getRight());
	MLRect progressRect = juceToMLRect(boundsRect);
	progressRect.setRight(xRange(progress));
	Rectangle<int> fullRect = MLToJuceRectInt(progressRect);
	Path fullBounds;
	fullBounds.addRectangle(fullRect);
	g.setColour(fc);	
	g.fillPath(fullBounds);	
	g.strokePath(gbounds, PathStrokeType(1.0f));

}

