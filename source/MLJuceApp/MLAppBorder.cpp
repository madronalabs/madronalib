
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppBorder.h"

MLAppBorder::MLAppBorder(MLAppView* pV) : 
	pMainView(pV),
    mpResizer(0),
	mGridUnitsX(0),
	mGridUnitsY(0),
	mZoomable(false)
{
	// don't buffer this big guy
	setBufferedToImage(false);
    setOpaque(true);
}

MLAppBorder::~MLAppBorder()
{
	delete mpResizer;
}

// build the resizer for target components that need them.
// Native Mac windows don't need resizers.
//
void MLAppBorder::makeResizer(Component* targetComp)
{
	// add the triangular resizer component for the bottom-right of the UI
	//getComponent()->addAndMakeVisible (mpResizer = new ResizableCornerComponent (targetComp, &myConstrainer));
	
	addAndMakeVisible (mpResizer = new ResizableCornerComponent (targetComp, &myConstrainer));
	mpResizer->setAlwaysOnTop(true);
}

void MLAppBorder::paint (Graphics& g)
{    
	// This is where most of the plugin's background is actually painted.
	// debug() << "MLAppBorder::paint\n";
	pMainView->getViewResources().mLookAndFeel.drawEntireBackground(g, mBorderRect);
}

int MLAppBorder::getHeightUnit()
{
	MLRect br = juceToMLRect(getBounds());	
	int windowHeight = br.height();		
	return (windowHeight / mGridUnitsY);
}

MLRect MLAppBorder::centerMainViewInWindow(int u)
{
	MLRect br = juceToMLRect(getBounds());
	
	int windowWidth = br.width();
	int windowHeight = br.height();	
	double windowRatio = (double)(windowWidth + 1)/(double)windowHeight;
	double viewRatio = (double)mGridUnitsX/(double)mGridUnitsY;
	
	int viewWidth, viewHeight;
	if ((!windowWidth) || (!windowHeight) || !u) return MLRect(0, 0, 64, 64);
	
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
    
	pMainView->getViewResources().mLookAndFeel.setGridUnitSize(u);	
	
	if ((!viewWidth) || (!viewHeight) || !u) return MLRect(0, 0, 64, 64);
	int vwq = viewWidth / u * u;
	int vhq = viewHeight / u * u;
	int borderX = (windowWidth - vwq)/2;
	int borderY = (windowHeight - vhq)/2;
	
	return MLRect(borderX, borderY, vwq, vhq);
}

void MLAppBorder::resized()
{
	int u = getHeightUnit();
	mBorderRect = centerMainViewInWindow(u); 
	if (pMainView) pMainView->resizeWidget(mBorderRect, u);

	Rectangle<int> bounds = getBounds();
	MLRect newBounds = juceToMLRect(bounds);

	int w = newBounds.width();
	int h = newBounds.height();

	// move resizer widget
	if(mpResizer)
	{
		mpResizer->setBounds (w - 16, h - 16, 16, 16);
	}
	
	AppViewResources& resources = pMainView->getViewResources();
	if(mBoundsRect != newBounds)
	{		
		resources.mLookAndFeel.makeBackgroundImage(newBounds, mBorderRect);
	}
	
	mBoundsRect = newBounds;
}

void MLAppBorder::setGridUnits(int gx, int gy)
{
	AppViewResources& resources = pMainView->getViewResources();
	resources.mLookAndFeel.setGridUnits(gx, gy);	
	myConstrainer.setFixedAspectRatio((float)gx/(float)gy);	
	mGridUnitsX = gx;
	mGridUnitsY = gy;
}

void MLAppBorder::setZoomable(bool z)
{ 
	mZoomable = z;
	myConstrainer.setZoomable(z); 
}

