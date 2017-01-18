// example of portaudio wrapping low-level madronalib DSP code.

#pragma once

#include "../source/DSP/MLDSP.h"
#include "MLProperty.h"
#include "MLPropertySet.h"
#include "MLProc.h"

using namespace ml;


class ProcMultiply : public Proc
{
	
public:
	
	//	MLProcInfoBase& procInfo() override { return mInfo; }
	
	static constexpr str_const paramNames[]{ "a", "b", "c" };
	static constexpr str_const inputNames[]{ "foo", "bar" };
	static constexpr str_const outputNames[]{ "baz" };
	
	// + 1 leaves room for setting when names are not found
	float params[countof(paramNames) + 1];
	DSPVector* inputs[countof(inputNames) + 1];
	DSPVector* outputs[countof(outputNames) + 1];
	
	inline float& param(str_const str)
	{
		return params[find(paramNames, str)];
	}
	
	inline DSPVector& input(str_const str)
	{
		return *inputs[find(inputNames, str)];
	}
	
	inline void setInput(str_const str, DSPVector &v) override
	{
		inputs[find(inputNames, str)] = &v;
	}
	
	inline DSPVector& output(str_const str)
	{
		return *outputs[find(outputNames, str)];
	}
	
	inline void setOutput(str_const str, DSPVector &v) override
	{
		outputs[find(outputNames, str)] = &v;
	}
	
	void test ()
	{
		std::cout << "counts: " << countof(ProcMultiply::paramNames) << " " << countof(ProcMultiply::inputNames) << "\n";
		std::cout << "finds: " << find(ProcMultiply::inputNames, "bar") << "\n";
		std::cout << paramNames[1] << paramNames[2] << "\n";
		
		std::cout << "params: " << param("a") << " " << param("b") << " " << param("c") << " " << param("d") << " " << "\n";
		param("a") = 1.29f;
		param("b") = 2.29f;
		param("c") = 3.29f;
		param("d") = 4.29f;
		std::cout << "params: " << param("a") << " " << param("b") << " " << param("c") << " " << param("d") << " " << "\n";
	}
	
	void process() override;
	
private:
	
	//	MLProcInfo<MLProcMultiplyAdd> mInfo;
	
};


