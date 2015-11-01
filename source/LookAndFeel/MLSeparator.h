
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SEPARATOR_HEADER__
#define __ML_SEPARATOR_HEADER__

#include "MLUI.h"


class MLSeparator  : public Component, public SettableTooltipClient
{
public:
    MLSeparator();
    ~MLSeparator();

    void setText (const String& text);

	enum ColourIds
    {
        backgroundColourId              = 0x1008900,   
        foregroundColourId              = 0x1008a00,  
    };

    //==============================================================================

protected:

    void paint (Graphics& g);

    void lookAndFeelChanged();

    void visibilityChanged();

    void colourChanged();

private:
	String displayedMessage, currentMessage;
    MLSeparator (const MLSeparator&);
    const MLSeparator& operator= (const MLSeparator&);
};


#endif  //__ML_SEPARATOR_HEADER__