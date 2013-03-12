
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_MATRIX_H
#define ML_PROC_MATRIX_H


#include "MLProc.h"

static const unsigned kMLMatrixMaxIns = 32;
static const unsigned kMLMatrixMaxOuts = 32;
extern const unsigned long MatrixConstants[16];

// compact form representing connections in a matrix. 
class MLMatrixConnectionList
{
public:
	unsigned size;
	unsigned char data[2 * kMLMatrixMaxIns * kMLMatrixMaxOuts];
	
	MLMatrixConnectionList(){};
	~MLMatrixConnectionList(){};
	
	bool operator== (const MLMatrixConnectionList& b);
	bool operator!= (const MLMatrixConnectionList& b);
};

// ----------------------------------------------------------------
// class definition


class MLProcMatrix : public MLProc
{
public:
	MLProcMatrix();
	~MLProcMatrix();
	
	MLProc::err resize();
	void clearConnections();
	void connect(unsigned a, unsigned b);
	void disconnect(unsigned a, unsigned b);
	bool getConnection(unsigned a, unsigned b);
	void getConnectionData(MLMatrixConnectionList* pData);

	void clear(){};
	void process(const int frames);		
	void calcCoeffs();
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcMatrix> mInfo;
	MLSample mGain[kMLMatrixMaxIns + 1][kMLMatrixMaxOuts + 1];
	
	
};

#endif // ML_PROC_MATRIX_H