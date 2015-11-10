
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
#if defined(DEBUG) || (BETA) || (DEMO)
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
	mpInputToSignalsProc = 0;
	mpHostPhasorProc = 0;
	clear();
	
	// TODO refactor - paths to proccs are plugin-specific
	if (makeMidiInput)
 	{
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
        
        // make any published signal outputs at top level only
        forEachXmlChildElement(*pRootElem, child)
        { 
            if (child->hasTagName("signal"))
            {
                int mode = eMLRingBufferMostRecent;
                MLPath procArg = RequiredPathAttribute(child, "proc");
                MLSymbol outArg = RequiredAttribute(child, "output");
                MLSymbol aliasArg = RequiredAttribute(child, "alias");
                
                if (procArg && outArg && aliasArg)
                {
                    int bufLength = kMLRingBufferDefaultSize;
                    bufLength = child->getIntAttribute("length", bufLength);
                    MLPath procPath (procArg);
                    MLSymbol outSym (outArg);
                    MLSymbol aliasSym (aliasArg);
                    publishSignal(procPath, outSym, aliasSym, mode, bufLength);
                }
            }
		}
		
		graphOK = true;
	}
	
	
	
	
	
	// if we made one or more Patchers with the right names in the document, save a list of them for direct access. 
	getProcList(mPatcherList, MLPath(kMLPatcherProcName), kMLEngineMaxVoices);

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

// prepareEngine() needs to be called if the sampling rate or block size changes.
//
MLProc::err MLDSPEngine::prepareEngine(double sr, int bufSize, int chunkSize)
{
	// debug() << " MLDSPEngine::prepareEngine: DSPEngine " << std::hex << (void *)this << std::dec << "\n";
	err e = OK;
    
	// set denormal state
	int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
	int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
	_mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
	// _mm_setcsr( oldMXCSR ); // restore MXCSR state (needed?)
    
	if ((mGraphStatus == OK) && (mCompileStatus == OK))
	{
		// set self as context to get size and rate chain started.
		setContext(this);	

		// connect input Signals and set sizes.
		for (int i=0; i < mInputChans; ++i)
		{
			mInputSignals[i]->setRate(sr);
			mInputSignals[i]->setDims(bufSize);
			clearInput(i+1);
			e = setInput(i+1, (*mInputSignals[i]));
			if (e != OK) goto bail;
		}		

		for (int i=0; i < mInputChans; ++i)
		{
			if (!mInputBuffers[i]->resize(bufSize)) 
			{
				// TODO use exceptions
				e = MLProc::memErr; 
				goto bail;
			}
		}		

		int outs = getNumOutputs();	
		for(int i=0; i < outs; ++i)
		{
			if (!mOutputBuffers[i]->resize(bufSize + chunkSize))
			{
				e = MLProc::memErr; 
				goto bail;
			}
			
			// add samples to ringbuffers so processing in chunks is always possible.
			MLSignal delay(chunkSize);
			delay.clear();		
			mOutputBuffers[i]->write(delay.getBuffer(), chunkSize);
		}
		
		mSamplesToProcess = 0; // doesn't count delay
		setSampleRate(sr);
		setBufferSize(bufSize);
		setVectorSize(chunkSize);

		// after setVectorSize, set midiToSignals input buffer size.
		if (mpInputToSignalsProc)
		{
            //debug() << "MLDSPEngine::prepareEngine: bufsize: " << bufSize << ", vecSize: " << vecSize << "\n";
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

void MLDSPEngine::setBufferSize(int size) 
{
	mBufferSize = size;
}

void MLDSPEngine::setInputChannels(int c) 
{
	mInputChans = c;
	mInputSignals.clear();
	for (int i=0; i<mInputChans; i++)
	{		
		mInputSignals.push_back(MLSignalPtr(new MLSignal(kMLProcessChunkSize))); 
		mInputBuffers.push_back(MLRingBufferPtr(new MLRingBuffer()));
	}
}

void MLDSPEngine::setOutputChannels(int c) 
{
	mOutputChans = c;
	for (int i=0; i<mOutputChans; i++)
	{		
		mOutputBuffers.push_back(MLRingBufferPtr(new MLRingBuffer()));
	}
}

// set pointers to client signal buffers.
void MLDSPEngine::setIOBuffers(const ClientIOMap& pMap)
{
    mIOMap = pMap;
}

// read from client input buffers to input ringbuffers.
void MLDSPEngine::writeInputBuffers(const int samples)
{
	for(int i=0; i<mInputChans; ++i)
	{
		mInputBuffers[i]->write(mIOMap.inputs[i], samples);
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
		mOutputBuffers[i]->write(getOutput(i+1).getBuffer(), samples);
	}
} 

// write outputs of root container to ringbuffers
void MLDSPEngine::clearOutputBuffers()
{
	int outs = getNumOutputs();
	for(int i=0; i < outs; ++i)
	{
		mOutputBuffers[i]->clear();
	}
} 

// read ringbuffers to client output buffers
void MLDSPEngine::readOutputBuffers(const int samples)
{
	int outs = getNumOutputs();
    int okToRead = true;
	for(int i=0; i < outs; ++i)
	{
		if(mOutputBuffers[i]->getRemaining() < samples)
        {
            okToRead = false;
            break;
        }
	}
    if(okToRead)
    {
        for(int i=0; i < outs; ++i)
        {
            if (samples != mOutputBuffers[i]->read(mIOMap.outputs[i], samples))
            {
                debug() << "MLDSPEngine: output ringbuffer out of data!\n";
            }
        }
    }
}

void MLDSPEngine::dump()
{
	dumpGraph(0);
}

// ----------------------------------------------------------------
#pragma mark published signals

void MLDSPEngine::publishSignal(const MLPath & procAddress, const MLSymbol outputName, const MLSymbol alias,
                                    int trigMode, int bufLength)
{
	err e = addSignalBuffers(procAddress, outputName, alias, trigMode, bufLength);
	if (e == OK)
	{
		MLProcList signalBuffers;
		gatherSignalBuffers(procAddress, alias, signalBuffers);
		if (signalBuffers.size() > 0)
		{
			// TODO list copy is unnecessary here -- turn this around
			mPublishedSignalMap[alias] = signalBuffers;
		}
	}
}

// return the number of buffers matching alias in the signal list.
// these are not always copies of a multiple signal, as when a wildcard is used, for example.
//
int MLDSPEngine::getPublishedSignalVoices(const MLSymbol alias)
{
	int nVoices = 0;
	
	// look up signal container
	MLPublishedSignalMapT::const_iterator it = mPublishedSignalMap.find(alias);
	if (it != mPublishedSignalMap.end())
	{
		const MLProcList& bufList = it->second;
		// count procs in buffer list
		for (MLProcList::const_iterator jt = bufList.begin(); jt != bufList.end(); jt++)
		{
			MLProcPtr proc = (*jt);
			if (proc)
			{
				nVoices++;
			}
		}
	}
	return nVoices;
}

// return the number of currently enabled buffers matching alias in the signal list.
//
int MLDSPEngine::getPublishedSignalVoicesEnabled(const MLSymbol alias)
{
	int nVoices = 0;
	
	// look up signal container
	MLPublishedSignalMapT::const_iterator it = mPublishedSignalMap.find(alias);
	if (it != mPublishedSignalMap.end())
	{
		const MLProcList& bufList = it->second;
		// count enabled procs in buffer list
		for (MLProcList::const_iterator jt = bufList.begin(); jt != bufList.end(); jt++)
		{
			MLProcPtr proc = (*jt);
			if (proc && proc->isEnabled())
			{
				nVoices++;
			}
		}
	}
    //debug() << "getPublishedSignalVoicesEnabled: " << alias << ": " << nVoices << "\n";
	return nVoices;
}

// get the buffer size for a published signal by looking at the length parameter
// of the first attached ring buffer.
//
int MLDSPEngine::getPublishedSignalBufferSize(const MLSymbol alias)
{
	int result = 0;
	
	// look up signal container
	MLPublishedSignalMapT::const_iterator it = mPublishedSignalMap.find(alias);
	if (it != mPublishedSignalMap.end())
	{
		const MLProcList& bufList = it->second;
		MLProcList::const_iterator jt = bufList.begin();
		MLProcPtr proc = (*jt);
		if (proc)
		{
			result = proc->getParam("length");
		}
	}
	return result;
}

// read samples from a published signal list into outSig.
// return the number of samples read.
//
int MLDSPEngine::readPublishedSignal(const MLSymbol alias, MLSignal& outSig)
{
	int nVoices = 0;
	int r;
	int minSamplesRead = 2<<16;
	int samples = outSig.getWidth();
	outSig.clear();
	outSig.setConstant(false);
	
	// look up signal container
	MLPublishedSignalMapT::const_iterator it = mPublishedSignalMap.find(alias);
	if (it != mPublishedSignalMap.end())
	{
		const MLProcList& bufList = it->second;
		
        // TODO why all this counting
		// iterate buffer list and count enabled ring buffers.
		for (MLProcList::const_iterator jt = bufList.begin(); jt != bufList.end(); jt++)
		{
			MLProcPtr proc = (*jt);
			if (proc && proc->isEnabled())
			{
				nVoices++;
			}
		}
		
		// read from enabled ring buffers into the destination signal.
        // if more than one voice is found, each voice goes into one row of the signal.
		// need to iterate here again so we can pass nVoices to readToSignal().
		if (nVoices > 0)
		{
			int voice = 0;  // check change
			for (MLProcList::const_iterator jt = bufList.begin(); jt != bufList.end(); jt++)
			{
				MLProcPtr proc = (*jt);
				if (proc && proc->isEnabled())
				{
					MLProcRingBuffer& bufferProc = static_cast<MLProcRingBuffer&>(*proc);
					r = bufferProc.readToSignal(outSig, samples, voice);
					minSamplesRead = min(r, minSamplesRead);
					voice++;
				}
			}
		}
	}
#ifdef ML_DEBUG
	else
	{
		debug() << "MLProcContainer::readPublishedSignal: signal " << alias << " not found in container " << getName() << "!\n";
	}
#endif
	return minSamplesRead;
}


// ----------------------------------------------------------------
#pragma mark MIDI


void MLDSPEngine::setEngineInputProtocol(int p)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setParam("protocol", p);
		
		if(p == kInputProtocolMIDI_MPE)
		{
			// just use max voices for MPE for now
			// later if we do splits as defined in the MPE spec this can change
			// mpInputToSignalsProc->setParam("voices", 15);
		}
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

// ----------------------------------------------------------------
#pragma mark Process

void MLDSPEngine::setCollectStats(bool k)
{
	mCollectStats = k;
}

// run one buffer of the compiled graph, processing signals from the global inputs (if any)
// to the global outputs.  Processes sub-procs in chunks of our preferred vector size.
//
void MLDSPEngine::processSignalsAndEvents(const int frames, const MLControlEventVector& events, const int64_t , const double secs, const double ppqPos, const double bpm, bool isPlaying)
{
	int sr = getSampleRate();
	int processed = 0;
	bool reportStats = false;
	osc::int64 startTime = 0, endTime = 0;
    MLControlEventVector::const_iterator firstEvent, lastEvent;
		
	//debug() << "new samples: " << frames << "\n";
	
    if (mpHostPhasorProc)
	{	
		mpHostPhasorProc->setTimeAndRate(secs, ppqPos, bpm, isPlaying);
	}	

	// count sample interval to collect stats
	if (mCollectStats)
	{
		mStatsCount += frames;
		const int statsInterval = 1;
		if (mStatsCount > sr * statsInterval)
		{	
			reportStats = true;
			mStatsCount -= sr * statsInterval;
		}
	}

	writeInputBuffers(frames);
	mSamplesToProcess += frames;

	// mVectorSize is set in MLPluginProcessor::prepareToPlay to kMLProcessChunkSize
	while(mSamplesToProcess >= mVectorSize)
	{
		readInputBuffers(mVectorSize);

		if (mpInputToSignalsProc)
		{
			// send events within time [processed, processed + mVectorSize] to processor
			mpInputToSignalsProc->clearEvents();
			for(auto& engineEvent : events)
			{
				int t = engineEvent.mTime;
				if(within(t, processed, processed + mVectorSize))
				{
					MLControlEvent e = engineEvent;
					e.mTime -= processed;
					mpInputToSignalsProc->addEvent(e);
				}
			}
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
			if (mCollectStats)
            {
                startTime = juce::Time::getHighResolutionTicks();
            }
			
			process(mVectorSize);  // MLProcContainer::process()
			
			if (mCollectStats) 
			{
				endTime = juce::Time::getHighResolutionTicks();
				mCPUTimeCount += juce::Time::highResolutionTicksToSeconds (endTime - startTime);
				mSampleCount += mVectorSize;
			}
		}		
         
        writeOutputBuffers(mVectorSize);
		processed += mVectorSize;
		mSamplesToProcess -= mVectorSize;
	}	
	readOutputBuffers(frames);
}



