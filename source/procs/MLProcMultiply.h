// TEMP - procs will just have a .cpp
// TODO make this a template

#pragma once

#include "MLProcFactory.h"

namespace ml{

class ProcMultiply : public Proc
{
public:	
	static constexpr constStr paramNames[3]{ "a", "b", "c" };
	static constexpr constStr textParamNames[1]{ "mode" };
	static constexpr constStr inputNames[2]{ "foo", "bar" };
	static constexpr constStr outputNames[1]{ "baz" };
	
	// Proc boilerplate
	static constexpr constStrArray pn_{paramNames};
	static constexpr constStrArray tn_{textParamNames};
	static constexpr constStrArray in_{inputNames};
	static constexpr constStrArray on_{outputNames};
	
	virtual const constStrArray& getParamNames() override { return pn_; }
	virtual const constStrArray& getTextParamNames() override { return tn_; }
	virtual const constStrArray& getInputNames() override { return in_; }
	virtual const constStrArray& getOutputNames() override { return on_; }
	
	// + 1 leaves room for setting when keys are not found.
	float params[constCount(paramNames) + 1];
	ml::TextFragment textParams[constCount(textParamNames) + 1];
	DSPVector* inputs[constCount(inputNames) + 1];
	DSPVector* outputs[constCount(outputNames) + 1];

	inline float& param(constStr str)
	{
		return params[constFind(paramNames, str)];
	}
	
	inline const TextFragment& textParam(constStr str)
	{
		return textParams[constFind(textParamNames, str)];
	}
	
	inline DSPVector& input(constStr str) 
	{ 
		return *inputs[constFind(inputNames, str)]; 
	}
	
	inline DSPVector& output(constStr str)
	{
		return *outputs[constFind(outputNames, str)];
	}
	
	// TODO remove and make non-const versions for external use
	// MLProc implementation (more boilerplate)
	virtual void setParam(constStr str, float v) override
	{
		params[constFind(paramNames, str)] = v;
	}
	virtual void setTextParam(constStr str, TextFragment v) override
	{
		textParams[constFind(textParamNames, str)] = v;
	}
	void setInput(constStr str, DSPVector &v) override
	{
		inputs[constFind(inputNames, str)] = &v;
	}
	void setOutput(constStr str, DSPVector &v) override
	{
		outputs[constFind(outputNames, str)] = &v;
	}
	
	void process() override;
};

} // namespace ml
