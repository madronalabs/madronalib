
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"
#include "MLRingBuffer.h"

#include "ffft/FFTRealFixLen.h"

const int kFFTBits = 10;
const int kFFTSize = 1 << kFFTBits;

// ----------------------------------------------------------------
// class definition

class MLProcSpectralPeak : public MLProc
{
public:
	MLProcSpectralPeak();
	~MLProcSpectralPeak();
	
	void clear();
	MLProc::err resize();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcSpectralPeak> mInfo;
	void calcCoeffs(void);
	
	float mThresh;
	
	// 1024-point (2^10) FFT object constructed.
	ffft::FFTRealFixLen <kFFTBits> mFFT;
	
	MLSignal mFFTIn, mFFTOut;

	
	MLRingBuffer mRingBuffer;
	

};

// ----------------------------------------------------------------
// registry section

namespace{
	
	MLProcRegistryEntry<MLProcSpectralPeak> classReg("spectral_peak");
	ML_UNUSED MLProcParam<MLProcSpectralPeak> params[] = {"thresh"};
	ML_UNUSED MLProcInput<MLProcSpectralPeak> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcSpectralPeak> outputs[] = {"peak"};
	
}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcSpectralPeak::MLProcSpectralPeak()
{
	mRingBuffer.resize(kFFTSize);
	mFFTIn.setDims(kFFTSize);
	mFFTOut.setDims(kFFTSize);
}

MLProcSpectralPeak::~MLProcSpectralPeak()
{
}

void MLProcSpectralPeak::clear()
{
	mThresh = 0.001f;
}

MLProc::err MLProcSpectralPeak::resize()
{
	debug() << "MLProcSpectralPeak: RESIZED\n";
	return OK;// MLTEST
}

// generate envelope output based on gate and control signal inputs.
void MLProcSpectralPeak::process(const int frames)
{	
	//float invSr = getContextInvSampleRate();
	const MLSignal& in = getInput(1);
	MLSignal& peak = getOutput(1);	
	
	if (mParamsChanged)
	{
		mParamsChanged = false;
	}
	
	for (int n = 0; n < frames; n++)
	{
		float fx = in[n];
		mRingBuffer.write(&fx, 1);
		
		if(mRingBuffer.getRemaining() >= kFFTSize)
		{
			mRingBuffer.readWithOverlap(mFFTIn.getBuffer(), kFFTSize, kFFTSize/2);
			
			const float* px = mFFTIn.getConstBuffer();
			float* py = mFFTOut.getBuffer();
			
			mFFT.do_fft (py, px);     // x (real) --FFT---> y (complex)
			
//			debug() << "MLProcSpectralPeak: PROCESSED " << kFFTSize << " samples: \n";
//			mFFTOut.dump(debug(), true);
			
		}
		
		peak[n] = MLRand();
		
	}	
	
//	debug() << "REM: " << mRingBuffer.getRemaining() << "\n";
	
	/*
	 int sr = getContextSampleRate();
	 mT += samples;
	 if (mT > sr)
	 {
	 //	debug() << getName() << " state: " << mState << " env: " << mEnv << "\n";
		debug() << "trig: " << trigSelect << "\n";
		mT -= sr;
	 }
	 */
}

// TODO envelope sometimes sticks on for very fast gate transients.  Rewrite this thing!



