
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_BUTTON_HEADER__
#define __ML_BUTTON_HEADER__

#include "MLUI.h"
#include "MLWidget.h"
#include "MLSymbol.h"
#include "MLLabel.h"

class MLButton : 
	public Button,   // TODO do not inherit all the crap in Button
	public MLWidget
{
friend class MLLookAndFeel;
public:
	MLButton (const String& label);
	MLButton ();
	
    enum ColourIds
    {
        buttonOnColourId                = 0x1009000,  
		buttonOffColourId               = 0x1009001,
        textColourId                    = 0x1009002
    };
		
	class Listener 
	{
	public:
		virtual ~Listener() {}
		virtual void buttonClicked (MLButton* button) = 0;
	};
	
    void setListener (MLButton::Listener* listener);
	
	// set all colors calculated from fill color
	virtual void setFillColor(const Colour c);
	
	void setClickSize(int x, int y);
	void setImage(const Image& m){ mImage = m; }
	void setImageOffset(int x, int y){ mImageOffset = Vec2(x, y); }
	void setLabelText (const char* ln);
	void sizeChanged();
    void clicked();
	void setRange(float a, float b) { mOffValue = a; mOnValue = b; }
	
	float getOffValue() { return mOffValue; }
	float getOnValue() { return mOnValue; }

	void resizeWidget(const MLRect& b, const int u);

protected:

	void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown);

	bool mDrawName;

	bool mDoRollover;
	Image mImage;
	float mMargin;
	MLPoint mImageOffset;
	MLPoint mLabelOffset;
	
	float mOffValue;
	float mOnValue;
    float mLineThickness;
	
    MLButton::Listener* mpListener;
	
private:	

};


#endif // __ML_BUTTON_HEADER__