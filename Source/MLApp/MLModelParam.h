
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL_PARAM__
#define __ML_MODEL_PARAM__

#include <string>
#include "MLSignal.h"
#include "MLDebug.h"

class MLModelParam
{
public:
	enum eType
	{
		kUndefinedParam	= 0,
		kFloatParam	= 1,
		kStringParam = 2,
		kSignalParam = 3
	};

	MLModelParam();
	MLModelParam(const MLModelParam& other);
	MLModelParam& operator= (const MLModelParam & other); 
	MLModelParam(float v);
	MLModelParam(const std::string& s);
	MLModelParam(const MLSignal& s);
	~MLModelParam();
			
	float getFloatValue() const;
	const std::string& getStringValue() const;
	const MLSignal& getSignalValue() const;

	void setValue(float v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	
	bool operator== (const MLModelParam& b) const;
	bool operator!= (const MLModelParam& b) const;
	eType getType() const { return mType; }
	
	bool operator<< (const MLModelParam& b) const;
	
private:	
	
	eType mType;
	union 
	{
		float mFloatVal;
		std::string* mpStringVal;
		MLSignal* mpSignalVal;
	} mVal;
};

std::ostream& operator<< (std::ostream& out, const MLModelParam & r);

#endif // __ML_MODEL_PARAM__