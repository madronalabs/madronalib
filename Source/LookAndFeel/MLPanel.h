
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PANEL_HEADER__
#define __ML_PANEL_HEADER__

#include "MLUI.h"
#include "MLWidget.h"

class MLPanel : 
	public Component,
	public MLWidget
{
public:
    MLPanel();
    ~MLPanel();

    void setText (const String& text);

	void setBackgroundColor(const Colour& color);

	enum ColourIds
    {
        backgroundColourId              = 0x1008900,   
        foregroundColourId              = 0x1008a00,  
    };

    //==============================================================================

protected:

    void paint (Graphics& g);
	void resizeWidget(const MLRect& b, const int);

private:
	String displayedMessage, currentMessage;
    MLPanel (const MLPanel&);
    const MLPanel& operator= (const MLPanel&);
};


#endif  //__ML_PANEL_HEADER__