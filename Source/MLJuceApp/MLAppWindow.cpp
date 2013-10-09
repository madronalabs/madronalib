
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppWindow.h" 

// --------------------------------------------------------------------------------
#pragma mark MLAppWindow

MLAppWindow::MLAppWindow()
	: DocumentWindow (MLProjectInfo::projectName,
					  Colour::fromHSV(0.5f, 0.0f, 0.30f, 1.f),
					  DocumentWindow::allButtons,
					  true),
	mpBorder(0),
	mpConstrainer(0)
{
 	setVisible (false);
    setOpaque(true);
    setResizable(true, false);
	setResizeLimits (400, 300, 8192, 8192);
	
	// note: native title bar prevents resizing by dragging border on Mac. 
	// also enforces fixed title bar height, and automatically includes a
	// resizing rect in lower right. 
	setUsingNativeTitleBar (true);
    
#if GLX
    openGLContext.attachTo (*getTopLevelComponent());
#endif
    
	mpConstrainer = new MLBoundsConstrainer();
	setConstrainer (mpConstrainer);
	//setVisible (true);
}

MLAppWindow::~MLAppWindow()
{
	// pView = 0;
	//  TODO  String getWindowStateAsString();
	//  TODO  bool restoreWindowStateFromString (const String& previousState);
	// (the content component will be deleted automatically, so no need to do it here)
	if (mpBorder)
	{
		delete mpBorder;
	}
	if (mpConstrainer)
	{
		delete mpConstrainer;
	}
#if GLX
    openGLContext.detach();
#endif
}

void MLAppWindow::mouseDown (const MouseEvent& e)
{
	myDragger.startDraggingComponent (this, e);
}

void MLAppWindow::mouseDrag (const MouseEvent& e)
{
	myDragger.dragComponent (this, e, nullptr);
}

void MLAppWindow::setGridUnits(double gx, double gy)
{
	mGridUnitsX = gx;
	mGridUnitsY = gy;
	mpConstrainer->setFixedAspectRatio(gx/gy);	
	
	if (mpBorder)
	{
		mpBorder->setGridUnits(gx, gy);
	}
}

void MLAppWindow::setContent(MLAppView* contentView)
{
	if (!mpBorder) 
	{
		mpBorder = new MLAppBorder();
		mpBorder->setBounds(getBounds());
	}
	setContentNonOwned (mpBorder, false);
	mpBorder->addMainView(contentView);

}

void MLAppWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}
