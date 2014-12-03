
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MENU_BUTTON_HEADER__
#define __ML_MENU_BUTTON_HEADER__


#include "MLUI.h"
#include "MLButton.h"
#include "MLStringUtils.h"

class MLMenuButton : 
	public MLButton
{
public:
     MLMenuButton();
    ~MLMenuButton();

    enum ColourIds
    {
        buttonColourId                  = 0x1000100,
		textColourId					= 0x1000102,
	};
	
    enum Style
    {
        kPlainStyle = 0,
		kRightArrowStyle,
		kTextOnlyStyle
	};
	
	void paint(Graphics& g);
	void clicked();
	void setStyle(int);
	
	// MLPropertyListener
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
protected:

private:
	int mStyle;
};


#endif   // __ML_MENU_BUTTON_HEADER__
