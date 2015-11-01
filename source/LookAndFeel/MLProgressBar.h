
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PROGRESS_BAR__
#define __ML_PROGRESS_BAR__

#include "MLUI.h"
#include "MLWidget.h"

class MLProgressBar : 
	public Component,
	public MLWidget
{
public:
    MLProgressBar();
    ~MLProgressBar();

protected:
    void paint (Graphics& g);
};

#endif  //__ML_PROGRESS_BAR__