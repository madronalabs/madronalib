
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

// ----------------------------------------------------------------
// class definition

class MLProcSpectralPeak : public MLProc
{
public:
	MLProcSpectralPeak();
	~MLProcSpectralPeak();
	
	void process(const int n);		 
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	void doParams();

	MLProcInfo<MLProcSpectralPeak> mInfo;
	float mCentroid;
	
	// 1024-point (2^10) FFT object constructed.
	ffft::FFTRealFixLen <kFFTBits> mFFT;
	
	MLSignal mFFTIn, mFFTOut, mWindow, mMagnitudes;
	MLRingBuffer mRingBuffer;
	MLBiquad mOutputFilter;
		
#if SEND_OSC
	ml::Clock mClock;
	ml::OSCSender mOSCSender;
#endif

};

// ----------------------------------------------------------------
// registry section

namespace{
	
	MLProcRegistryEntry<MLProcSpectralPeak> classReg("spectral_peak");
	//ML_UNUSED MLProcParam<MLProcSpectralPeak> params[] = {};
	ML_UNUSED MLProcInput<MLProcSpectralPeak> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcSpectralPeak> outputs[] = {"peak"};
	
}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcSpectralPeak::MLProcSpectralPeak() :
	mCentroid(-16.f)
{
	mRingBuffer.resize(kFFTSize);
	mFFTIn.setDims(kFFTSize);
	mFFTOut.setDims(kFFTSize);
	mWindow.setDims(kFFTSize);
	
	// Generate a Hann window.
	// TODO move to dsp utils
	int w = mWindow.getWidth();
	for (int i = 0; i < w; ++i)
	{
		mWindow[i] = 0.5f * (1 - cosf(kMLTwoPi*((float)i/(float)w)));
	}
	
	// vertical signal: 1 frame of FFT data
	mMagnitudes.setDims(1, kFFTSize/2);
	
#if SEND_OSC
	mOSCSender.open(9000);
#endif
}

MLProcSpectralPeak::~MLProcSpectralPeak()
{
}

void MLProcSpectralPeak::doParams()
{
	float sr = getContextSampleRate();
	mOutputFilter.setSampleRate(sr);
	mOutputFilter.setOnePole(10.f);
}

// generate envelope output based on gate and control signal inputs.
void MLProcSpectralPeak::process(const int frames)
{	
	float sr = getContextSampleRate();
	const MLSignal& in = getInput(1);
	MLSignal& peak = getOutput(1);	
	
	bool doSend = false;
	
	if (mParamsChanged)
	{
		doParams();
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
			
			float* px = mFFTIn.getBuffer();
			float* py = mFFTOut.getBuffer();
			
			// window
			mFFTIn.multiply(mWindow);
			
			// get complex FFT
			mFFT.do_fft (py, px);     // x (real) --FFT---> y (complex)
			
			// get magnitudes
			int reals = kFFTSize/2;
			float magSum = 0.000001f;
			for(int bin=0; bin<reals; ++bin)
			{
				float i = py[bin];
				float j = py[bin+reals];
				float mag = sqrtf(i*i + j*j);
				mMagnitudes(0, bin) = mag;
				magSum += mag;
			}
			
			// apply threshold, below which centroid does not change
			if(magSum > 10.f) 
			{				
				// compute centroid, offset by 1 so bin 0 can get involved
				float binSum = 0.0f;
				for(int bin = 0; bin < reals; ++bin)
				{
					float mag = mMagnitudes(0, bin);
					binSum += mag*(bin + 1);
				}			
				float c = binSum/magSum - 1;
				c = clamp(c, 0.f, (reals + 0.f));
				float hzPerBin = sr/kFFTSize;
				float centroidHz = c*hzPerBin;
				float logPitch = log2f(centroidHz/440.f + 0.000001f);
				mCentroid = logPitch;
			}
			
			doSend = true;
		}
		
		peak[n] = mOutputFilter.processSample(mCentroid);

//	debug() << "REM: " << mRingBuffer.getRemaining() << "\n";
	
	}	
	
	if(doSend)
	{
#if SEND_OSC		
//		uint64_t ntpTime = mClock.now();
		mMagnitudes.setRate(sr);
		
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
#endif
		
	}
}

