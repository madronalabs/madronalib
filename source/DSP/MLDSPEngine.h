
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

const int kMLEngineMaxChannels = 8;

extern const char * kMLInputToSignalProcName;
extern const char * kMLHostPhasorProcName;
extern const char * kMLPatcherProcName;
extern const MLPath kMLPatcherPath;

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
	void removeGraphAndInputs(void);
	err getGraphStatus(void) {return mGraphStatus;}
	
	// ----------------------------------------------------------------
	#pragma mark graph dynamics
	//
	
	void compileEngine();
	bool getCompileStatus(void) {return mCompileStatus;}
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
	
	// ----------------------------------------------------------------
	// Housekeeping
			
	// test, display whole graph
	void dump(); 
	
	// ----------------------------------------------------------------
	// published signals

	void publishSignal(const MLPath & procName, const MLSymbol outputName, const MLSymbol alias, int trigMode, int bufLength);
	int getPublishedSignalVoices(const MLSymbol alias);
	int getPublishedSignalVoicesEnabled(const MLSymbol alias);
	int getPublishedSignalBufferSize(const MLSymbol alias);
	int readPublishedSignal(const MLSymbol alias, MLSignal& outSig);
    
	// ----------------------------------------------------------------
	// control input

	void setEngineInputProtocol(int p);
	void setInputDataRate(int p);
	void setInputFrameBuffer(PaUtilRingBuffer* pBuf);
	
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
	MLProcList mPatcherList;

	int mInputChans;
	int mOutputChans;
	
    ClientIOMap mIOMap;

    // map to published signals by name
	typedef std::map<MLSymbol, MLProcList> MLPublishedSignalMapT;
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
	err mCompileStatus;
	
	// keep track of buffered samples to process, not including one-vector delay.
	int mSamplesToProcess;
	int mStatsCount;
	int mSampleCount;
	double mCPUTimeCount;
		
	void writeInputBuffers(const int samples);
    void clearOutputBuffers();
	void readInputBuffers(const int samples);
	void writeOutputBuffers(const int samples);
	void readOutputBuffers(const int samples);
};



#endif

