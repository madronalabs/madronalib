
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_LABEL_HEADER__
#define __ML_LABEL_HEADER__

#include "MLUI.h"
#include "MLDSP.h"
#include "MLWidget.h"
#include <string>
	
typedef enum
{
	imageModeOpaque = 0,
	imageModeAlphaBrush,
	imageModeMultiply
}	eMLImageMode;

class MLLabel : 
	public Component,
	public MLWidget
{
friend class MLLookAndFeel;
public:
    enum ColourIds
    {
        backgroundColourId     = 0x1000980, 
        textColourId           = 0x1000981, 
    };

	static const int kInfWidth = 100000;
	
    MLLabel (const char* labelText = 0);
    ~MLLabel();

    void setFont (const Font& newFont);
    void setText (const char* newText);
	
	void setSizeMultiplier(float f) { mSizeMultiplier = f; }
	void resizeWidget(const MLRect& b, const int u);
	
	void setInverse(bool i);
	void setImage(const Image& m);
	void setImageData(const void* m) {mImageData = (char *)m;}

	void setDrawable (const Drawable* pImg);
	
	void setImageMode(eMLImageMode mode);
	void setJustification(const Justification& j);
	void setResizeToText(bool r) { mResizeToText = r; }

	void resized();
	
	void setStringAttribute(MLSymbol attr, const std::string& val);
	
protected:
    Font mFont;
	String mText;
	AttributedString mRichStr;
    Justification mJustification;
	float mSizeMultiplier;	
	void paint (Graphics& g);
	bool mInverse;
	bool mDrawImage;
	Image mImage;
	char* mImageData;	
	ScopedPointer<Drawable> mpDrawable;
	eMLImageMode mImageMode;
	MLRect mTextRect;
	bool mResizeToText;

private:
};



#endif