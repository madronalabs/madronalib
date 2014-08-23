
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_WAVEFORM_HEADER__
#define __ML_WAVEFORM_HEADER__

#include "MLUI.h"
#include "MLLookAndFeel.h"

class MLWaveform  :
public Component,
public MLWidget
{
public:
    MLWaveform();
    ~MLWaveform();
    
	enum ColourIds
    {
        foregroundColor              = 0x10150a01,
    };
	
	void triggerChangeMessage (const bool synchronous);    
	void resizeWidget(const MLRect& b, const int u);
    void setSignalToView(MLSignal* sig);
    
protected:
    
    void paint (Graphics& g);
    void lookAndFeelChanged();
    void visibilityChanged();
    void colourChanged();
    
private:
	float mOutlineThickness;
	
    MLWaveform (const MLWaveform&);
    const MLWaveform& operator= (const MLWaveform&);
    
    MLSignal* mpSignal;
};


#endif  //__ML_WAVEFORM_HEADER__