
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
     MLMenuButton ();
    ~MLMenuButton();
	void setAttribute(MLSymbol attr, float val);
	void setStringAttribute(MLSymbol , const std::string& val);

    enum ColourIds
    {
        buttonColourId                  = 0x1000100,
		textColourId					= 0x1000102, 
	};
	
	class Listener 
	{
	public:
		virtual ~Listener() {}
		virtual void showMenu (MLSymbol, MLMenuButton*) {}
	};
	
    void setListener (MLMenuButton::Listener* listener) { mpListener = listener; }
	// if true, draw left justified text with right arrow
	void setMenuTextStyle(bool t) { mMenuTextStyle = t; }
	
protected:
	void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown);
    void colourChanged();
	void mouseDown(const MouseEvent& e);

private:
	MLMenuButton::Listener* mpListener;
	bool mMenuTextStyle;
};


#endif   // __ML_MENU_BUTTON_HEADER__
