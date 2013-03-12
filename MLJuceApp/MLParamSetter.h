
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PARAM_SETTER_H__
#define __ML_PARAM_SETTER_H__

#include "MLSymbol.h"

class MLParamSetter 
{
public:
	MLParamSetter ();
	~MLParamSetter ();

	MLSymbol getParamName() { return mParamName; }
	void setParamName(MLSymbol p) {  mParamName = p; }

protected:

private:	

	MLSymbol mParamName;

};


#endif // __ML_PARAM_SETTER_H__