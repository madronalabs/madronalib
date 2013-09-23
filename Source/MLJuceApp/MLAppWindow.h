// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_JUCE_APP_WINDOW__
#define __ML_JUCE_APP_WINDOW__

#include "JuceHeader.h"
#include "MLAppView.h"
#include "MLAppBorder.h"
#include "MLBoundsConstrainer.h"

class MLAppWindow  : public DocumentWindow
{
public:
    MLAppWindow();
    ~MLAppWindow();

	void mouseDown (const MouseEvent& e);
	void mouseDrag (const MouseEvent& e);	
	void setGridUnits(double gx, double gy);
	void setContent(MLAppView* newView);
    void closeButtonPressed();

private:
 	ComponentDragger myDragger;
	
	float mGridUnitsX;
	float mGridUnitsY;
	
	MLAppBorder* mpBorder;
    MLBoundsConstrainer* mpConstrainer;
#if GLX
    juce::OpenGLContext openGLContext;
#endif
 };


#endif // __ML_JUCE_APP_WINDOW__

