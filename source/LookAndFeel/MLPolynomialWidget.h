
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_GRAPH_HEADER__
#define __ML_GRAPH_HEADER__

#include "MLDSP.h"
#include "MLUI.h"
#include "MLLookAndFeel.h"

#include <vector>

class MLPolynomialWidget : public Component
{
friend class MLLookAndFeel;
public:
    MLPolynomialWidget (const String& componentName = String::empty,
           const String& labelText = String::empty);
    ~MLPolynomialWidget();
	
	void setColor(const Colour& color);
	void setDomain(float a, float b);
	void setRange(float a, float b);
	void setPolyCoeffs(std::vector<float>& coeffs);

protected:
	void resized();
	void paint (Graphics& g);

private:
	// range over which the function is calculated
	MLRange mDomain;
	
	// coeffs if polynomial function
	std::vector<float> mPolyCoeffs;
	
	// ranges over which the function is drawn
	MLRange mViewDomain;
	MLRange mViewRange;
	int mResolution;
};

#endif // __ML_GRAPH_HEADER__