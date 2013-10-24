
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
	mpConstrainer(0),
    mUsingGL(false)
{
    setResizable(true, false);
	setResizeLimits (400, 300, 8192, 8192);

    //commandManager.registerAllCommandsForTarget (&mBorder);
    commandManager.registerAllCommandsForTarget (JUCEApplication::getInstance());
    
    // this lets the command manager use keypresses that arrive in our window to send
    // out commands
    addKeyListener (commandManager.getKeyMappings());
    

    setContentOwned(&mBorder, false);
	mpConstrainer = new MLBoundsConstrainer();
	setConstrainer (mpConstrainer);

	setUsingNativeTitleBar (true);
    
    
    // tells our menu bar model that it should watch this command manager for
    // changes, and send change messages accordingly.
    //&mBorder->setApplicationCommandManagerToWatch (&commandManager);
    

    //setVisible (true);
}

MLAppWindow::~MLAppWindow()
{
	// pView = 0;
	//  TODO  String getWindowStateAsString();
	//  TODO  bool restoreWindowStateFromString (const String& previousState);
	// (the content component will be deleted automatically, so no need to do it here)

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
    mBorder.setGridUnits(gx, gy);
}

void MLAppWindow::setContent(MLAppView* contentView)
{
	mBorder.addMainView(contentView);
}

void MLAppWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}

void MLAppWindow::moved()
{
    // repainting here is not quite perfect because
    // a flash happens when changing between
    // screens of differing resolution.
    
    // TODO could check for move to different display and only repaint in that case
    repaint();
    DocumentWindow::moved();
    mBorder.moved();
}

void MLAppWindow::resized()
{
    DocumentWindow::resized();
}

void MLAppWindow::setUsingOpenGL(bool b)
{
#if GLX
    if(b != mUsingGL)
    {
        if(b)
        {
            openGLContext.attachTo (*getTopLevelComponent());
        }
        else
        {
            openGLContext.detach();            
        }
        mUsingGL = b;
    }
#endif
}
