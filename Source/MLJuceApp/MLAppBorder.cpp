
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppBorder.h"

MLAppBorder::MLAppBorder() : 
	pMainView(0),
    mpResizer(0),
	mZoomable(false),
	mGridUnitsX(0),
	mGridUnitsY(0)
{
	MLWidget::setComponent(this);

	setWidgetBounds(MLRect(0, 0, 0, 0));
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);	
	setWidgetName("border");
    getComponent()->setOpaque(true);
}

MLAppBorder::~MLAppBorder()
{
	getComponent()->deleteAllChildren();
}

void MLAppBorder::addMainView(MLAppView* pView)
{
	pMainView = pView;
	getComponent()->addAndMakeVisible(pView);
}

// build the resizer for target components that need them.
// Native Mac windows don't need resizers.
//
void MLAppBorder::makeResizer(Component* targetComp)
{
	// add the triangular resizer component for the bottom-right of the UI
    getComponent()->addAndMakeVisible (mpResizer = new ResizableCornerComponent (targetComp, &myConstrainer));
	mpResizer->setAlwaysOnTop(true);
}

void MLAppBorder::paint (Graphics& g)
{    
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	
    // TODO optimize, because this has to be redrawn behind every non-opaque component repaint.
	// This is where most of the plugin's background is actually painted.
		
	MLRect br = getWidgetLocalBounds(); // top left should be at 0, 0 anyway
	Rectangle<int> jr = MLToJuceRectInt(br);
 
	/*
	g.setColour(Colours::red);
	MLRect localBounds = getWidgetLocalBounds();
	g.fillRect (MLToJuceRectInt(localBounds));
	localBounds.shrink(10.f);
	g.setColour(Colours::blue);
	g.fillRect (MLToJuceRectInt(localBounds));
	 */
	
	debug() << "MLAppBorder: PAINTING " << br << " border " << mBorderRect << "\n";
	

	//int yOffset = mBorderRect.top();
	myLookAndFeel->drawBackgroundRectAtOffset(g, this, br, mBorderRect.getTopLeft());
	
	myLookAndFeel->drawUnitGridRectAtOffset(g, this, br, mBorderRect.getTopLeft());
	
	/*
	// TEST outline border areas
	Path p, p2;
	int w = getWidth();
	int h = getHeight();
	p.addRectangle(0, 0, w, h);
	
	if (pMainView)
	{
		p2.addRectangle(pMainView->getBounds());
	}
	else
	{
		// draw X
		p2.startNewSubPath(0, 0);
		p2.lineTo(w, h);
		p2.startNewSubPath(w, 0);
		p2.lineTo(0, h);
	}
	g.setColour(Colours::blue);
	g.strokePath(p, PathStrokeType(1.f));
	g.setColour(Colours::red);
	g.strokePath(p2, PathStrokeType(1.f));
	*/
}

void MLAppBorder::centerMainViewInWindow()
{
	MLRect br = getWidgetBounds();
	int windowWidth = br.width();
	int windowHeight = br.height();	
	double windowRatio = (double)(windowWidth + 1)/(double)windowHeight;
	double viewRatio = (double)mGridUnitsX/(double)mGridUnitsY;
	int u = (int)(windowHeight / mGridUnitsY);
	int viewWidth, viewHeight;
	if ((!windowWidth) || (!windowHeight) || !u) return;
	
	// TODO different modes: fit fixed scale, quantize only. 
	// This is fit fixed scale, good for static layouts.
	if(windowRatio > viewRatio)
	{
		// too wide
		viewHeight = windowHeight;
		viewWidth = floor((double)windowHeight*viewRatio);
	}
	else
	{
		// too tall
		viewWidth = windowWidth;
		viewHeight = floor((double)windowWidth/viewRatio);
		u = viewWidth / mGridUnitsX;
	}
    
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnitSize(u);	

	if ((!viewWidth) || (!viewHeight) || !u) return;
	int vwq = viewWidth / u * u;
	int vhq = viewHeight / u * u;
	int borderX = (windowWidth - vwq)/2;
	int borderY = (windowHeight - vhq)/2;
	
	mBorderRect = MLRect(borderX, borderY, vwq, vhq);
	if (pMainView) pMainView->resizeWidget(mBorderRect, u);
	
	// MLTEST
	debug() << "MLAppBorder border rect: " << mBorderRect << "\n";
}

void MLAppBorder::resized()
{
	centerMainViewInWindow();
	
	// move resizer widget
	if(mpResizer)
	{
		int w = getWidgetBounds().width();
		int h = getWidgetBounds().height();
		mpResizer->setBounds (w - 16, h - 16, 16, 16);
		// MLTEST
		debug() << "MLAppBorder: resized to " << w << ", " << h << "\n";
	}
	
}

void MLAppBorder::setGridUnits(int gx, int gy)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGridUnits(gx, gy);	
	myConstrainer.setFixedAspectRatio((float)gx/(float)gy);	
	mGridUnitsX = gx;
	mGridUnitsY = gy;
}

void MLAppBorder::setContent(MLAppView* contentView)
{
	// set content of border to view
	addMainView(contentView);
}

void MLAppBorder::setZoomable(bool z)
{ 
	mZoomable = z;
	myConstrainer.setZoomable(z); 
}

