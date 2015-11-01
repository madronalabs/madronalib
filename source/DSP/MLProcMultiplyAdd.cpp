
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
//#include <xmmintrin.h>

// ----------------------------------------------------------------
// class definition


class MLProcMultiplyAdd : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcMultiplyAdd> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcMultiplyAdd> classReg("multiply_add");
	ML_UNUSED MLProcInput<MLProcMultiplyAdd> inputs[] = {"m1", "m2", "a1"};
	ML_UNUSED MLProcOutput<MLProcMultiplyAdd> outputs[] = {"out"};
}	


// ----------------------------------------------------------------
// implementation

/*

void MLProcMultiplyAdd::process(const int frames)
{
	const MLSignal& m1 = getInput(1);
	const MLSignal& m2 = getInput(2);
	const MLSignal& a1 = getInput(3);
	MLSignal& out = getOutput();

	for (int n=0; n<frames; ++n)
	{
		out[n] = m1[n]*m2[n] + a1[n];
	}
	
	out.setConstant(false);
}
*/

/*
void MLProcMultiplyAdd::process(const int frames)
{
	const MLSignal& m1 = getInput(1);
	const MLSignal& m2 = getInput(2);
	const MLSignal& a1 = getInput(3);
	MLSignal& out = getOutput();

	bool km1 = m1.isConstant();
	bool km2 = m2.isConstant();
	bool ka1 = a1.isConstant();

	out.setConstant(km1 && km2 && ka1); 

	if (km1 || km2 || ka1) // can't use SSE if we have any constant signals.
	{	
		for (int n=0; n<frames; ++n)
		{
			out[n] = m1[n]*m2[n] + a1[n];
		}
	}
	else
	{	
		const MLSample* pm1 = m1.getConstBuffer();
		const MLSample* pm2 = m2.getConstBuffer();
		const MLSample* pa1 = a1.getConstBuffer();
		MLSample* pout = out.getBuffer();

		__m128 vm1, vm2, va1, vr; 
		int c = frames >> kMLSamplesPerSSEVectorBits;
		
		for (int v = 0; v < c; ++v)
		{
			vm1 = _mm_load_ps(pm1);
			vm2 = _mm_load_ps(pm2);
			va1 = _mm_load_ps(pa1);
			vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
			_mm_store_ps(pout, vr);
			pm1 += kSSEVecSize;
			pm2 += kSSEVecSize;
			pa1 += kSSEVecSize;
			pout += kSSEVecSize;
		}
	}
}
*/


void MLProcMultiplyAdd::process(const int frames)
{
	const MLSignal& m1 = getInput(1);
	const MLSignal& m2 = getInput(2);
	const MLSignal& a1 = getInput(3);
	MLSignal& out = getOutput();
	
	bool km1 = m1.isConstant();
	bool km2 = m2.isConstant();
	bool ka1 = a1.isConstant();

	out.setConstant(false);
	
	
	// if either one of the multipliers is constant and 0., we just return a1.
	if ((km1 && (m1[0] == 0.f)) || (km2 && (m2[0] == 0.f)))
	{
		if (ka1)
		{
			out.setToConstant(a1[0]);
		}
		else
		{
			for (int n=0; n<frames; ++n)
			{
				out[n] = a1[n]; // TODO SSE copy signal method, no copy needed if in place.
			}
		}
		return;
	}
	
	const MLSample* pm1 = m1.getConstBuffer();
	const MLSample* pm2 = m2.getConstBuffer();
	const MLSample* pa1 = a1.getConstBuffer();
	MLSample* pout = out.getBuffer();
	__m128 vm1, vm2, va1, vr; 
	int c = frames >> kMLSamplesPerSSEVectorBits;

	const int mode = (km1 << 2) + (km2 << 1) + ka1;
	
	switch(mode)
	{
		case 0:	// none
			for (int v = 0; v < c; ++v)
			{
				vm1 = _mm_load_ps(pm1);
				vm2 = _mm_load_ps(pm2);
				va1 = _mm_load_ps(pa1);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm1 += kSSEVecSize;
				pm2 += kSSEVecSize;
				pa1 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 1: // ka1 constant
			va1 = _mm_set1_ps(a1[0]);
			for (int v = 0; v < c; ++v)
			{
				vm1 = _mm_load_ps(pm1);
				vm2 = _mm_load_ps(pm2);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm1 += kSSEVecSize;
				pm2 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 2:
			vm2 = _mm_set1_ps(m2[0]);
			for (int v = 0; v < c; ++v)
			{
				vm1 = _mm_load_ps(pm1);
				va1 = _mm_load_ps(pa1);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm1 += kSSEVecSize;
				pa1 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 3:
			vm2 = _mm_set1_ps(m2[0]);
			va1 = _mm_set1_ps(a1[0]);
			for (int v = 0; v < c; ++v)
			{
				vm1 = _mm_load_ps(pm1);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm1 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 4:
			vm1 = _mm_set1_ps(m1[0]);
			for (int v = 0; v < c; ++v)
			{
				vm2 = _mm_load_ps(pm2);
				va1 = _mm_load_ps(pa1);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm2 += kSSEVecSize;
				pa1 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 5:
			vm1 = _mm_set1_ps(m1[0]);
			va1 = _mm_set1_ps(a1[0]);
			for (int v = 0; v < c; ++v)
			{
				vm2 = _mm_load_ps(pm2);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pm2 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 6:
			vm1 = _mm_set1_ps(m1[0]);
			vm2 = _mm_set1_ps(m2[0]);
			for (int v = 0; v < c; ++v)
			{
				va1 = _mm_load_ps(pa1);
				vr = _mm_add_ps(_mm_mul_ps(vm1, vm2), va1);
				_mm_store_ps(pout, vr);
				pa1 += kSSEVecSize;
				pout += kSSEVecSize;
			}
		break;
		case 7: // hooray
			out.setToConstant(m1[0]*m2[0] + a1[0]);	
		break;
	
	}
}

