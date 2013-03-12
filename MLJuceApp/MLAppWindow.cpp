
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppWindow.h" 


MLAppWindow::MLAppWindow()
	: DocumentWindow (ProjectInfo::projectName,
					  Colour::fromHSV(0.5f, 0.0f, 0.30f, 1.f),
					  DocumentWindow::allButtons,
					  true),
	mpBorder(0)
{
	// note: native title bar prevents resizing from border on Mac. 
	// also enforces fixed title bar height.
	setUsingNativeTitleBar (true);
	setTitleBarHeight (kMLJuceTitleBarHeight);	
	setResizable(true, false);
	setConstrainer (&myConstrainer);
	setVisible (true);
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
}

void MLAppWindow::setGridUnits(double gx, double gy)
{
	mGridUnitsX = gx;
	mGridUnitsY = gy;
	myConstrainer.setTitleBarHeight(kMLJuceTitleBarHeight);
	myConstrainer.setFixedAspectRatio(gx/gy);	
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
	}
	ResizableWindow::setContentNonOwned (mpBorder, false);
	mpBorder->addMainView(contentView);
}

void MLAppWindow::closeButtonPressed()
{
	// When the user presses the close button, we'll tell the app to quit. This
	// MLAppWindow object will be deleted by the MLJuceApp class.
	JUCEApplication::quit();
}

void MLAppWindow::mouseDown (const MouseEvent& e)
{
	myDragger.startDraggingComponent (this, e);
}

void MLAppWindow::mouseDrag (const MouseEvent& e)
{
	myDragger.dragComponent (this, e, nullptr);
}
