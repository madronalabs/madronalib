
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_DSP_ENGINE_H
#define ML_DSP_ENGINE_H

#include "MLProcContainer.h"
#include "MLProcInputToSignals.h"
#include "MLInputProtocols.h"
#include "MLProcHostPhasor.h"
#include "MLProcMatrix.h"
#include "MLSignal.h"
#include "MLRingBuffer.h"
#include "OscTypes.h"

const unsigned kMLEngineMaxChannels = 8;

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
    struct IOPtrs
	{
		float * channel[kMLEngineMaxChannels];
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
	MLProc::err prepareToPlay(double sr, unsigned bufSize, unsigned vecSize);

	// ----------------------------------------------------------------
	// I/O
			
	// set size of I/O buffers. 
	void setBufferSize(unsigned size);
	unsigned getBufferSize(void){ return mBufferSize; }

	void setInputChannels(unsigned c); 
	void setOutputChannels(unsigned c); 
	
	// set external buffers for top level I/O with client
	void setIOPtrs(IOPtrs * pIns, IOPtrs * pOuts);
	
	// ----------------------------------------------------------------
	// Housekeeping
			
	// test, display whole graph
	void dump(); 
	
	// ----------------------------------------------------------------
	// control input

	void setInputProtocol(int p);
	void setInputDataRate(int p);
	void setInputFrameBuffer(PaUtilRingBuffer* pBuf);
	void clearMIDI();		
	void addNoteOn(unsigned note, unsigned vel, unsigned time);
	void addNoteOff(unsigned note, unsigned vel, unsigned time);
	void setController(unsigned controller, unsigned value, unsigned time);
	void setPitchWheel(unsigned value, unsigned time);
	void setAfterTouch(unsigned note, unsigned value, unsigned time);
	void setChannelAfterTouch(unsigned value, unsigned time);
	void setSustainPedal(int value, unsigned time);
	
	MLScale* getScale();
	
	// ----------------------------------------------------------------
	// Patcher

	MLProcList& getPatcherList();

	// ----------------------------------------------------------------
	// Process
	
public:

	void setCollectStats(bool k);

	// run the compiled graph, processing signals from the global inputs (if any)
	// to the global outputs. 
	void processBlock(const int samples, const int64_t samplesPos, const double secs, const double position, const double bpm, bool isPlaying); 

private:
	
	// ----------------------------------------------------------------
	// data
	
	// a pointer to the signal generator we might make in buildGraphAndInputs()
	MLProcInputToSignals* mpInputToSignalsProc;
	
	// same for a host sync phasor
	MLProcHostPhasor* mpHostPhasorProc;
	
	// list of patcher procs
	MLProcList mPatcherList;

	int mInputChans;
	int mOutputChans;
	
	// signal inputs
	IOPtrs mpIns; 
	
	// signal outputs
	IOPtrs mpOuts;
	
	// input signals that will be sent to the root proc.
	std::vector<MLSignalPtr> mInputSignals;

	// ring buffers so that processing can always
	// be done in multiples of 4 samples.
	std::vector<MLRingBufferPtr> mInputBuffers;
	std::vector<MLRingBufferPtr> mOutputBuffers;

	bool mCollectStats;
	unsigned mBufferSize;
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

