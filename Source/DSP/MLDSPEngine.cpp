
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDSPEngine.h"

const char * kMLInputToSignalProcName("the_midi_inputs");
const char * kMLHostPhasorProcName("the_host_phasor");
const char * kMLPatcherProcName("voices/voice/patcher");

MLDSPEngine::MLDSPEngine() : 
	mpInputToSignalsProc(0),
	mpHostPhasorProc(0),
	mInputChans(0),
	mOutputChans(0),
	mCollectStats(false),
	mBufferSize(0),
	mGraphStatus(unknownErr),
	mCompileStatus(unknownErr),
	mSamplesToProcess(0),
	mStatsCount(0),
	mSampleCount(0),
	mCPUTimeCount(0.)
{
#if defined(DEBUG) || defined(BETA) || (DEMO)
	//mCollectStats = true;
#endif
	setName("dspengine");
}

MLDSPEngine::~MLDSPEngine()
{
	removeGraphAndInputs();
}

// ----------------------------------------------------------------
#pragma mark build graph


MLProc::err MLDSPEngine::buildGraphAndInputs(juce::XmlDocument* pDoc, bool makeSignalInputs, bool makeMidiInput)
{
	MLProc::err r = unknownErr;
	bool graphOK = false;
	bool patcherOK = false;
	mpInputToSignalsProc = 0;
	mpHostPhasorProc = 0;
	clear();
	
	if (makeSignalInputs) // TODO for effects
	{
		
	}
	
	// TODO refactor
	if (makeMidiInput)
 	{
		debug() << "building MIDI input... ";
		// make XML node describing MIDI to signal processor. 
		juce::ScopedPointer<juce::XmlElement> pElem (new juce::XmlElement("proc"));
		pElem->setAttribute("class", "midi_to_signals");
		pElem->setAttribute("name", juce::String(kMLInputToSignalProcName));
		pElem->setAttribute("voices", (int)kMLEngineMaxVoices);			
		
		// build processor object.
		MLProc::err bpe = buildProc(pElem);
				
		// save a pointer to it.
		if (bpe == OK)
		{
			MLProcPtr pms = getProc(MLPath(kMLInputToSignalProcName));
			if (pms)
			{
				mpInputToSignalsProc = static_cast<MLProcInputToSignals*>(&(*pms));
				debug() << "MIDI / OSC input OK.\n";
			}
		}
	}	

	// make host sync phasor
	{
		juce::ScopedPointer<juce::XmlElement> pElem (new juce::XmlElement("proc"));
		pElem->setAttribute("class", "host_phasor");
		pElem->setAttribute("name", kMLHostPhasorProcName);
		
		// build processor object.
		MLProc::err bpe = buildProc(pElem);
				
		// save a pointer to it.
		if (bpe == OK)
		{
			MLProcPtr pms = getProc(MLPath(kMLHostPhasorProcName));
			if (pms)
			{
				mpHostPhasorProc = static_cast<MLProcHostPhasor*>(&(*pms));
			}
		}
	}
	
	juce::ScopedPointer<juce::XmlElement> pRootElem (pDoc->getDocumentElement());

	if (pRootElem)
	{	
		makeRoot("root");
		buildGraph(pRootElem);
		graphOK = true;
	}
	
	// if we made one or more Patchers with the right names in the document, save a list of them for direct access. 
	getProcList(mPatcherList, MLPath(kMLPatcherProcName), kMLEngineMaxVoices);
	if (!mPatcherList.empty())
	{
		patcherOK = true;
		//debug() << "got " << mPatcherList.size() << "patchers. \n";
	}

	if (graphOK)
	{
		r = OK;
		mGraphStatus = OK;
	}
	
	return r;
}

void MLDSPEngine::removeGraphAndInputs(void)
{
	mGraphStatus = unknownErr;
	mCompileStatus = unknownErr;
	{
	// unimplemented
	}
}

// ----------------------------------------------------------------
#pragma mark compile

void MLDSPEngine::compileEngine()
{
	err e = OK;
	
	// order procs and make connections
	// also makes connected signals
	compile();
	
	if (e != OK)
	{
		printErr(e);	
	}  
	else
	{
		mCompileStatus = OK;
	}
}


// prepareToPlay() needs to be called if the sampling rate or block size changes.
//
MLProc::err MLDSPEngine::prepareToPlay(double sr, unsigned bufSize, unsigned vecSize)
{
	// debug() << " MLDSPEngine::prepareToPlay: DSPEngine " << std::hex << (void *)this << std::dec << "\n"; 
	err e = OK;
	
	if ((e == OK) && (mGraphStatus == OK) && (mCompileStatus == OK))
	{
		// set self as context to get size and rate chain started.
		setContext(this);	

		// connect input Signals and set sizes.
		for (int i=0; i < mInputChans; ++i)
		{
			mInputSignals[i]->setRate((MLSampleRate)sr);
			mInputSignals[i]->setDims(bufSize);
			clearInput(i+1);
			e = setInput(i+1, (*mInputSignals[i]));
			if (e != OK) goto bail;
		}		

		for (int i=0; i < mInputChans; ++i)
		{
			if (!mInputBuffers[i]->resize(bufSize)) 
			{
				e = MLProc::memErr; 
				goto bail;
			}
		}		

		unsigned outs = getNumOutputs();	
		for(unsigned i=0; i < outs; ++i)
		{
			if (!mOutputBuffers[i]->resize(bufSize + vecSize))
			{
				e = MLProc::memErr; 
				goto bail;
			}
			
			// add samples to ringbuffers so processing in vector size chunks is always possible.
			MLSignal delay(vecSize);	
			delay.clear();		
			mOutputBuffers[i]->write(delay.getBuffer(), vecSize);
		}
		
		mSamplesToProcess = 0; // doesn't count delay
		setSampleRate((MLSampleRate)sr);
		setBufferSize(bufSize);
		setVectorSize(vecSize);

		// after setVectorSize, set midiToSignals input buffer size.
		if (mpInputToSignalsProc)
		{
// TEMP
debug() << "MLDSPEngine::prepareToPlay: mpInputToSignalsProc bufsize: " << bufSize << "\n";
			mpInputToSignalsProc->setParam("bufsize", bufSize);
			mpInputToSignalsProc->resize();		
		}
				
		e = prepareToProcess();		
		clear();
	}
bail:
	if (e != OK)
	{
		printErr(e);	
	} 
	/* 
	else
	{
		setEnabled(true);
	}
	*/
	return e;
}


// ----------------------------------------------------------------
#pragma mark I/O



void MLDSPEngine::setBufferSize(unsigned size) 
{
	mBufferSize = size;
}

void MLDSPEngine::setInputChannels(unsigned c) 
{
	mInputChans = c;
	mInputSignals.clear();
	for (int i=0; i<mInputChans; i++)
	{		
		mInputSignals.push_back(MLSignalPtr(new MLSignal())); 
		mInputBuffers.push_back(MLRingBufferPtr(new MLRingBuffer()));
	}
}

void MLDSPEngine::setOutputChannels(unsigned c) 
{
	mOutputChans = c;
	for (int i=0; i<mOutputChans; i++)
	{		
		mOutputBuffers.push_back(MLRingBufferPtr(new MLRingBuffer()));
	}
}

// set pointers to client signal buffers.
void MLDSPEngine::setIOPtrs(IOPtrs * pIns, IOPtrs * pOuts) 
{
	mpIns = *pIns;
	mpOuts = *pOuts;
}

// read from client input buffers to input ringbuffers.
void MLDSPEngine::writeInputBuffers(const int samples)
{
	for(int i=0; i<mInputChans; ++i)
	{
		mInputBuffers[i]->write(mpIns.channel[i], samples);
	}
}

// read input ringbuffers to input signals
void MLDSPEngine::readInputBuffers(const int samples)
{
	for(int i=0; i<mInputChans; ++i)
	{
		if (samples != mInputBuffers[i]->read(mInputSignals[i]->getBuffer(), samples))
		{
			debug() << "MLDSPEngine: input ringbuffer out of data!\n";
		}
	}
}

// write outputs of root container to ringbuffers
void MLDSPEngine::writeOutputBuffers(const int samples)
{
	int outs = getNumOutputs();	
	for(int i=0; i < outs; ++i)
	{
//        getOutput(i+1).write(mpOuts.channel[i], offset, samples);
		
		mOutputBuffers[i]->write(getOutput(i+1).getBuffer(), samples);
	}
} 

// read ringbuffers to client output buffers
void MLDSPEngine::readOutputBuffers(const int samples)
{
	int outs = getNumOutputs();	
	for(int i=0; 
	i < outs; ++i)
	{
		if (samples != mOutputBuffers[i]->read(mpOuts.channel[i], samples))
		{
			debug() << "MLDSPEngine: output ringbuffer out of data!\n";
		}
	}
} 

void MLDSPEngine::dump()
{
	dumpGraph(0);
}


// ----------------------------------------------------------------
#pragma mark MIDI


void MLDSPEngine::setInputProtocol(int p)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setParam("protocol", p);
	}

}

void MLDSPEngine::setInputDataRate(int p)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setParam("data_rate", p);
	}
}

// set frame buffer for OSC inputs
void MLDSPEngine::setInputFrameBuffer(PaUtilRingBuffer* pBuf)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setInputFrameBuffer(pBuf);
	}
	else 
	{
		debug() << "MLDSPEngine::setInputFrameBuffer: no mpInputToSignalsProc!\n";
	}
}

void MLDSPEngine::clearMIDI()
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->clearMIDI();
	}
}

void MLDSPEngine::addNoteOn(unsigned note, unsigned vel, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->addNoteOn(note, vel, time);
	}
}

void MLDSPEngine::addNoteOff(unsigned note, unsigned vel, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->addNoteOff(note, vel, time);
	}
}

void MLDSPEngine::setController(unsigned controller, unsigned value, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setController(controller, value, time);
	}
}

void MLDSPEngine::setPitchWheel(unsigned value, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setPitchWheel(value, time);
	}
}

void MLDSPEngine::setAfterTouch(unsigned note, unsigned value, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setAfterTouch(note, value, time);
	}
}

void MLDSPEngine::setChannelAfterTouch(unsigned value, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setChannelAfterTouch(value, time);
	}
}

void MLDSPEngine::setSustainPedal(int value, unsigned time)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setSustainPedal(value, time);
	}
}

MLScale* MLDSPEngine::getScale()
{
	MLScale* r = 0;
	if (mpInputToSignalsProc)
	{
		r = mpInputToSignalsProc->getScale();
	}
	return r;
}


// ----------------------------------------------------------------
#pragma mark Patcher

MLProcList& MLDSPEngine::getPatcherList()
{
	return mPatcherList;
}


// ----------------------------------------------------------------
#pragma mark Process

void MLDSPEngine::setCollectStats(bool k)
{
	mCollectStats = k;
}

// run one buffer of the compiled graph, processing signals from the global inputs (if any)
// to the global outputs.  Processes sub-procs in chunks of our preferred vector size.
void MLDSPEngine::processBlock(const int newSamples, const int64_t , const double secs, const double ppqPos, const double bpm, bool isPlaying)
{
	int sr = getSampleRate();
	int processed = 0;
	bool reportStats = false;
	osc::int64 startTime, endTime;
		
	if (mpHostPhasorProc)
	{	
		mpHostPhasorProc->setTimeAndRate(secs, ppqPos, bpm, isPlaying);
	}	

	// count sample interval to collect stats
	if (mCollectStats)
	{
		mStatsCount += newSamples;
		const int statsInterval = 1;
		if (mStatsCount > sr * statsInterval)
		{	
			reportStats = true;
			mStatsCount -= sr * statsInterval;
		}
	}

	writeInputBuffers(newSamples);
	mSamplesToProcess += newSamples;
	
	//debug() << "new samples: " << newSamples << "\n";

	// set denormal state
	int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
	int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
	_mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR

	while(mSamplesToProcess >= mVectorSize)
	{
		readInputBuffers(mVectorSize);
		
		// set MIDI signals offset into change lists
		if (mpInputToSignalsProc)
		{
			mpInputToSignalsProc->setMIDIFrameOffset(processed);
		}
		
		if (reportStats)
		{
			MLSignalStats stats;
			collectStats(&stats);
			process(mVectorSize);  // MLProcContainer::process()
	
			debug() << "\n";
			debug() << "processed " << mSampleCount << " samples in " << mCPUTimeCount << " seconds,"
				<< "vector size " << mVectorSize << ".\n";
			double uSecsPerSample = mCPUTimeCount / (double)mSampleCount * 1000000.;
			double maxuSecsPerSample = getInvSampleRate() * 1000000.;
			double CPUFrac = uSecsPerSample / maxuSecsPerSample;
			double percent = CPUFrac * 100.;
			debug() << (int)(mCPUTimeCount / (double)mVectorSize * 1000000.) << " microseconds per sample (";
			debug() << std::fixed;
			debug() << std::setprecision(1);
			debug() << percent << "\%)\n";
			
			// clear time and sample counters
			mCPUTimeCount = 0.;
			mSampleCount = 0;
			
			collectStats(0); // turn off stats collection
			debug() << "\n";
			stats.dump();
			reportStats = false;
		}
		else
		{
			if (mCollectStats) startTime = juce::Time::getHighResolutionTicks();
			
			process(mVectorSize); 
			
			if (mCollectStats) 
			{
				endTime = juce::Time::getHighResolutionTicks();
				mCPUTimeCount += juce::Time::highResolutionTicksToSeconds (endTime - startTime);
				mSampleCount += mVectorSize;
			}
		}
		
		// TEST
		//debug() << "samples to process:" << mSamplesToProcess << "(" << c++ << ")\n";	
		
		writeOutputBuffers(mVectorSize);
		processed += mVectorSize;
		mSamplesToProcess -= mVectorSize;

	}	
	readOutputBuffers(newSamples);

	_mm_setcsr( oldMXCSR ); // restore MXCSR state
}



