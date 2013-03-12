
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppBorder.h"

MLBoundsConstrainer::MLBoundsConstrainer() : mTitleBarHeight(0)
{
	setMinimumSize(480, 360);
}

MLBoundsConstrainer::~MLBoundsConstrainer()
{
}

/*
void MLBoundsConstrainer::checkBounds (MLRect& bounds,
	const MLRect&,
	const MLRect&,
	 bool ,
	 bool ,
	 bool ,
	 bool ) 
{
	int minHeight = 300;
 	int maxHeight = 2000;

	double a = getFixedAspectRatio();

debug() << "a: " << a << "\n";

	// constrain the aspect ratio if one has been specified..
    if (a > 0.0)
    {
		int h = bounds.getHeight();
		h = clamp(h, minHeight, maxHeight);
		bounds.setHeight(h);
		
		int w = ((double)(h - mTitleBarHeight))*a; //correct for title bar
		bounds.setWidth(w);		
		
debug() << "out:" << w << " by " << h << "\n";
	}
}
*/

// --------------------------------------------------------------------------------
#pragma mark MLAppBorder

MLAppBorder::MLAppBorder() : 
	mpResizer(0),
	pMainView(0)
{
	Component::setBounds(0, 0, 0, 0);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);	
	Component::setName("border");
}

MLAppBorder::~MLAppBorder()
{
	deleteAllChildren();
}

void MLAppBorder::addMainView(MLAppView* pView)
{
	pMainView = pView;
	addAndMakeVisible(pView);
}

// build the resizer for target components that need them
//
void MLAppBorder::makeResizer(Component* targetComp)
{
	// add the triangular resizer component for the bottom-right of the UI
    addAndMakeVisible (mpResizer = new ResizableCornerComponent (targetComp, &myConstrainer));
	mpResizer->setAlwaysOnTop(true);
}

void MLAppBorder::paint (Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->drawBackground(g, this);	
	
	/*
	// TEST outline border areas
	g.setColour(Colours::red);
	Path p;
	int w = getWidth();
	int h = getHeight();
	p.addRectangle(0, 0, w, h);
	if (pMainView)
	{
		p.addRectangle(pMainView->getBounds());
	}
	else
	{
		p.startNewSubPath(0, 0);
		p.lineTo(w, h);
		p.startNewSubPath(w, 0);
		p.lineTo(0, h);
	}
	g.strokePath(p, PathStrokeType(0.5f));
	*/
}

void MLAppBorder::centerMainViewInWindow()
{
	Rectangle<int> br = getBounds();
	int u = (int)(getHeight() / mGridUnitsY);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnitSize(u);	
	int vw = br.getWidth();
	int vh = br.getHeight();
	
	if ((!vw) || (!vh) || !u) return;
	int vwq = vw / u * u;
	int borderLR = (vw - vwq)/2;
	
	// move so mGridUnitsX * gridUnitSize is centered horizontally in window
	if (pMainView)
		pMainView->resizeWidget(MLRect(borderLR, 0, vwq, vh), u);
}

void MLAppBorder::resized()
{
	centerMainViewInWindow();
	
	// move resizer widget
	if(mpResizer)
	{
		int w = getWidth();
		int h = getHeight();
		mpResizer->setBounds (w - 16, h - 16, 16, 16);
	}
}

void MLAppBorder::setGridUnits(double gx, double gy)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnits(gx, gy);	
	mGridUnitsX = gx;
	mGridUnitsY = gy;
	myConstrainer.setTitleBarHeight(0);
	myConstrainer.setFixedAspectRatio(gx/gy);	
}

void MLAppBorder::setContent(MLAppView* contentView)
{
	// set content of border to view
	addMainView(contentView);
}


