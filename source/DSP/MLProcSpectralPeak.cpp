
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"
#include "MLRingBuffer.h"
#include "MLClock.h"
#include "MLOSCSender.h"

#include "ffft/FFTRealFixLen.h"

const int kFFTBits = 9;
const int kFFTSize = 1 << kFFTBits;

#define SEND_OSC	1
const int kMaxOSCBlobSize = 64;

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
	
	MLSignal mFFTIn, mFFTOut, mMagnitudes;
	MLSignal mFFTResult;
	
	MLRingBuffer mRingBuffer;
	
	int mTest;
	
	
#if SEND_OSC
	ml::Clock mClock;
	ml::OSCSender mOSCSender;
#endif

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
	mMagnitudes.setDims(1, kFFTSize/2);
#if SEND_OSC
	mOSCSender.open(9000);
#endif
	
	mTest = 0;
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
	
	// 2D signal for results
	mFFTResult.setDims(kFFTSize);

	return OK;// MLTEST
}

// generate envelope output based on gate and control signal inputs.
void MLProcSpectralPeak::process(const int frames)
{	
	//float invSr = getContextInvSampleRate();
	const MLSignal& in = getInput(1);
	MLSignal& peak = getOutput(1);	
	
	bool doSend = false;
	
	
	if (mParamsChanged)
	{
		mParamsChanged = false;
	}
	
	for (int n = 0; n < frames; n++)
	{
		float fx = in[n];
		mRingBuffer.write(&fx, 1);
		
		// TODO better syntax for overlap processing in buffer
		if(mRingBuffer.getRemaining() >= kFFTSize)
		{
			mRingBuffer.readWithOverlap(mFFTIn.getBuffer(), kFFTSize, kFFTSize/2);
			
			const float* px = mFFTIn.getConstBuffer();
			float* py = mFFTOut.getBuffer();
			
			// get complex FFT
			mFFT.do_fft (py, px);     // x (real) --FFT---> y (complex)
			
			// get magnitudes
			int reals = kFFTSize/2;
			for(int bin=0; bin<reals; ++bin)
			{
				float i = py[bin];
				float j = py[bin+reals];
				mMagnitudes(0, bin) = sqrtf(i*i + j*j);
			}
			
//			debug() << "MLProcSpectralPeak: PROCESSED " << kFFTSize << " samples: \n";
//			mFFTOut.dump(debug(), true);
			
			
			doSend = true;
		}
		
		peak[n] = 0.5f;

//	debug() << "REM: " << mRingBuffer.getRemaining() << "\n";
	
	}	
	
	
	int sr = getContextSampleRate();
	mMagnitudes.setRate(sr);
	
	int interval= sr/10;
	mTest += frames;
	if (mTest > interval)
	{
		mTest -= interval;
	}
	
	if(doSend)
	{
#if SEND_OSC		
//		uint64_t ntpTime = mClock.now();
		
		// send proc name as address
		std::string address = std::string("/signal/FFT");
		
		// get Blob with signal 
		// TODO buffer
		
		mOSCSender.getStream() << osc::BeginBundle(getContextTime())
		<< osc::BeginMessage( address.c_str() ) 
		<< mMagnitudes
		<< osc::EndMessage
		<< osc::EndBundle;
		
		mOSCSender.sendDataToSocket();	
		debug() << ".";
#endif
		
	}

}

