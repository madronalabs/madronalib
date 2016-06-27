
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_DSP_ENGINE_H
#define ML_DSP_ENGINE_H

#include "MLProcContainer.h"
#include "MLProcInputToSignals.h"
#include "MLInputProtocols.h"
#include "MLProcHostPhasor.h"
#include "MLSignal.h"
#include "MLRingBuffer.h"
#include "MLControlEvent.h"
#include "OscTypes.h"
#include "MLDSPDeprecated.h"

const int kMLEngineMaxChannels = 8;

extern const char * kMLInputToSignalProcName;
extern const char * kMLHostPhasorProcName;
extern const char * kMLPatcherProcName;

// MLDSPEngine: the bridge between a top-level MLProcContainer and the outside world.
// implemented as a special kind of MLProcContainer.  There will probably only be one
// MLDSPEngine in a given application, plug-in, etc.
//
class MLDSPEngine : public MLProcContainer
{
public:
    struct ClientIOMap
	{
		const float * inputs[kMLEngineMaxChannels];
		float * outputs[kMLEngineMaxChannels];
	};
	
	MLDSPEngine();
	~MLDSPEngine();	
	
	MLProc::err buildGraphAndInputs(juce::XmlDocument* pDoc, bool makeSignalInputs, bool makeMidiInput);
	err getGraphStatus(void) {return mGraphStatus;}
	
	// ----------------------------------------------------------------
	#pragma mark graph dynamics
	
	void compileEngine();
	bool getCompileStatus(void) {return mCompileStatus;}
	void setCompileStatus(bool b) {mCompileStatus = b;}
	MLProc::err prepareEngine(double sr, int bufSize, int chunkSize);

	// ----------------------------------------------------------------
	// I/O
			
	// set size of I/O buffers. 
	void setBufferSize(int size);
	int getBufferSize(void){ return mBufferSize; }

	void setInputChannels(int c); 
	void setOutputChannels(int c); 
	
	// set external buffers for top level I/O with client
	void setIOBuffers(const ClientIOMap& pMap);
	
	// as a Container, limit signal I/O to outside world connections
	virtual int getMaxInputSignals() { return mInputChans; }
	virtual int getMaxOutputSignals() { return mOutputChans; }
	
	// ----------------------------------------------------------------
	// Housekeeping
			
	// test, display whole graph
	void dump(); 
	
	// ----------------------------------------------------------------
	// published signals

	void publishSignal(const ml::Path & procName, const ml::Symbol outputName, const ml::Symbol alias, int trigMode, int bufLength, int frameSize = 1);
	
	int getPublishedSignalBufferSize(const ml::Symbol alias);
	int getPublishedSignalVoices(const ml::Symbol alias);

	int getPublishedSignalVoicesEnabled(const ml::Symbol alias);
	int readPublishedSignal(const ml::Symbol alias, MLSignal& outSig);
    
	// ----------------------------------------------------------------
	// control input

	void setEngineInputProtocol(int p);
	void setInputDataRate(int p);
	void setInputFrameBuffer(PaUtilRingBuffer* pBuf);
	void setMasterVolume(float v);
	
	// ----------------------------------------------------------------
	// Process

	void setCollectStats(bool k);

	// run the compiled graph, processing signals from the global inputs (if any)
	// to the global outputs. 
	void processSignalsAndEvents(const int samples, const MLControlEventVector& events, const int64_t samplesPos, const double secs, const double position, const double bpm, bool isPlaying);

private:
	
	// ----------------------------------------------------------------
	// data
	
	// a pointer to the signal generator we might make in buildGraphAndInputs()
	// TODO this should be in the graph definition instead
	// but everything needs hooking up right
	MLProcInputToSignals* mpInputToSignalsProc;
	
	// same for a host sync phasor
	MLProcHostPhasor* mpHostPhasorProc;
	
	// list of patcher procs
	// MLProcList mPatcherList; // MLTEST unused?

	int mInputChans;
	int mOutputChans;
	
	float mMasterVolume;
	
    ClientIOMap mIOMap;

    // map to published signals by name
	typedef std::map<ml::Symbol, MLProcList> MLPublishedSignalMapT;
	MLPublishedSignalMapT mPublishedSignalMap;
    
	// input signals that will be sent to the root proc.
	std::vector<MLSignalPtr> mInputSignals;

	// ring buffers so that processing can always
	// be done in multiples of 4 samples.
	std::vector<MLRingBufferPtr> mInputBuffers;
	std::vector<MLRingBufferPtr> mOutputBuffers;

	bool mCollectStats;
	int mBufferSize;
	err mGraphStatus;
	bool mCompileStatus;
	
	// keep track of buffered samples to process, not including one-vector delay.
	int mSamplesToProcess;
	int mStatsCount;
	int mSampleCount;
	double mCPUTimeCount;
		
	void writeInputBuffers(const int samples);
    void clearOutputBuffers();
	void readInputBuffers(const int samples);
	void multiplyOutputBuffersByVolume();
	void writeOutputBuffers(const int samples);
	void readOutputBuffers(const int samples);
	void clearOutputs(int frames);
	
	MLBiquad mMasterVolumeFilter;
	MLSignal mMasterVolumeSig;
};



#endif

