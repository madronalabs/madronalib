
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MENU_BUTTON_HEADER__
#define __ML_MENU_BUTTON_HEADER__


#include "MLUI.h"
#include "MLButton.h"

class MLMenuButton : 
	public MLButton
{
protected:

public:
     MLMenuButton();
    ~MLMenuButton();

    enum ColourIds
    {
        buttonColourId                  = 0x1000100,
		textColourId					= 0x1000102, 
	};
	
	void paint(Graphics& g);
	void clicked();

	// if true, draw left justified text with right arrow
	void setMenuTextStyle(bool t) { mMenuTextStyle = t; }
	
	// MLPropertyListener
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
protected:

private:
	bool mMenuTextStyle;
};


#endif   // __ML_MENU_BUTTON_HEADER__
