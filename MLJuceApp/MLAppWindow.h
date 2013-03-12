
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_JUCE_APP_WINDOW__
#define __ML_JUCE_APP_WINDOW__

#include "JuceHeader.h"
#include "MLAppView.h"
#include "MLAppBorder.h"

const int kMLJuceTitleBarHeight = 24;

class MLAppWindow  : public DocumentWindow
{
public:
    MLAppWindow();
    ~MLAppWindow();
	void closeButtonPressed();
	void mouseDown (const MouseEvent& e);
	void mouseDrag (const MouseEvent& e);
	
	void setGridUnits(double gx, double gy);
	void setContent(MLAppView* newView);

private:
 	ComponentDragger myDragger;
	
	float mGridUnitsX;
	float mGridUnitsY;
	
	MLAppBorder* mpBorder;
    MLBoundsConstrainer myConstrainer;
 };

#endif // __ML_JUCE_APP_WINDOW__


/*
// can't include windows.h before juce, thus
// can't rely on JUCE_WIN32 to be defined
// to know if we're under windows or not

#if (defined (_WIN32) || defined (_WIN64))
#include <windows.h>
#endif

#include <juce.h>

// For mac it is safe to include these headers
// after juce, so we can safely use JUCE_MAC here

#if JUCE_MAC
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif


#if JUCE_WIN32

String getCurrentExecutableName()
{
    WCHAR buf[MAX_PATH] = { '\0' };

    if ( !GetModuleFileNameW( NULL, buf, MAX_PATH ))
    {
        return String::empty;    // Error
    }
    return String(buf);
}

#elif JUCE_MAC

String getCurrentExecutableName() throw()
{
    char str[2*MAXPATHLEN] = {'\0'};

    unsigned int size = 2*MAXPATHLEN;
    if ( _NSGetExecutablePath( str, &size ))
    {
       return String::empty;    // Error
    }

    return String(str, size);
}

#else
#error unsupported OS
#endif 

*/