
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcMatrix.h"

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcMatrix> classReg("matrix");
	// no parameters
	ML_UNUSED MLProcParam<MLProcMatrix> params[4] = { "inputs", "outputs", "in", "out" };	
	ML_UNUSED MLProcInput<MLProcMatrix> inputs[] = {"*"};	// variable inputs "in1", "in2", ...
	ML_UNUSED MLProcOutput<MLProcMatrix> outputs[] = {"*"};	// variable outputs "out1", "out2", ...
}

// ----------------------------------------------------------------
// MLMatrixConnectionList

bool MLMatrixConnectionList::operator== (const MLMatrixConnectionList& b)
{
	if (size != b.size)
	{
		return false;
	}
	
	for(unsigned i=0; i<2 * kMLMatrixMaxIns * kMLMatrixMaxOuts; ++i)
	{
		if (data[i] != b.data[i])
		{
			return false;
		}
	}
	return true;
}
	
bool MLMatrixConnectionList::operator!= (const MLMatrixConnectionList& b)
{
	return !operator==(b);
}

// ----------------------------------------------------------------
// implementation

MLProcMatrix::MLProcMatrix()
{
	setParam("in", 0.);
	setParam("out", 0.);
//	debug() << "MLProcMatrix constructor\n";
	clearConnections();
}

MLProcMatrix::~MLProcMatrix()
{
//	debug() << "MLProcMatrix destructor\n";
}


MLProc::err MLProcMatrix::resize()
{
	MLProc::err e = OK;

//	const unsigned inputs = getParam("inputs"); // currently unused
//	const unsigned outputs = getParam("outputs");
	
//debug() << "MLProcMatrix: " << inputs << " inputs " << outputs << " outputs \n";
	
	return e;
}

void MLProcMatrix::clearConnections()
{
//debug() << "MLProcMatrix::MLProcMatrix::clearConnections! \n";
	for (unsigned i=1; i <= kMLMatrixMaxIns; ++i)
	{
		for (unsigned j=1; j <= kMLMatrixMaxOuts; ++j)
		{
			mGain[i][j] = 0.;
		}
	}
}

// multiple connections are made here using connect method.
void MLProcMatrix::connect(unsigned a, unsigned b)
{
	const unsigned inputs = getNumInputs();
	const unsigned outputs = getNumOutputs();
	if ((a <= inputs) && (b <= outputs))
	{
		mGain[a][b] = 1.;
	}
}

void MLProcMatrix::disconnect(unsigned a, unsigned b)
{
	const unsigned inputs = getNumInputs();
	const unsigned outputs = getNumOutputs();
	if ((a <= inputs) && (b <= outputs))
	{
		mGain[a][b] = 0.;
	}
}

// get a single connection.
bool MLProcMatrix::getConnection(unsigned a, unsigned b)
{
	bool r = false;
	const unsigned inputs = getNumInputs();
	const unsigned outputs = getNumOutputs();
	if ((a <= inputs) && (b <= outputs))
	{
		r = (mGain[a][b] > 0.5f);
	}
	return r;
}

// put info about every connection into the destination block of memory.
// For each connection a pair of bytes [a, b] goes into the destination.
void MLProcMatrix::getConnectionData(MLMatrixConnectionList* pData)
{
	unsigned n = 0;	
	const unsigned inputs = getNumInputs();
	const unsigned outputs = getNumOutputs();
	
	for(unsigned i=0; i<2 * kMLMatrixMaxIns * kMLMatrixMaxOuts; ++i)
	{
		pData->data[i] = 0;
	}
	
	for (unsigned i=1; i <= inputs; ++i)
	{
		for (unsigned j=1; j <= outputs; ++j)
		{
			if (mGain[i][j] > 0.5f)
			{
				pData->data[n*2] = (unsigned char)i;
				pData->data[n*2 + 1] = (unsigned char)j;
				n++;
			}
		}
	}
	
	pData->size = n;
}


// single connections are made here by parameters.
void MLProcMatrix::calcCoeffs()
{
	const int in = (int)getParam("in");
	const int out = (int)getParam("out");
	const int use_in = (in > 0);
	const int use_out = (out > 0);
	const int mode = (use_in << 1) + use_out;
	
	switch(mode)
	{
		case 3:	// both 
			clearConnections();
			connect(in, out);
		break;
		case 2: // in to 1
			clearConnections();
			connect(in, 1);
		break;
		case 1: // 1 to out
			clearConnections();
			connect(1, out);
		break;
		case 0:
		default:
		break;
	}
	
	mParamsChanged = false;
}

void MLProcMatrix::process(const int frames)
{
	const unsigned inputs = getNumInputs();
	const unsigned outputs = getNumOutputs();
	
	if (mParamsChanged)
	{
		calcCoeffs();
	}
	
	// TODO optimize, calc constants
	
	for (unsigned j=1; j <= outputs; ++j)
	{
		MLSignal& y = getOutput(j);
		y.clear();
		for (unsigned i=1; i <= inputs; ++i)
		{
			const MLSignal& x = getInput(i);
			if (mGain[i][j] > 0.)
			{
				for (int n=0; n < frames; ++n)
				{
					y[n] += x[n];
				}
			}
		}
	}
}



