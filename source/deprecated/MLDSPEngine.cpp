
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDSPEngine.h"
#include "MLProcRingBuffer.h"


const char * kMLInputToSignalProcName("the_midi_inputs");
const char * kMLHostPhasorProcName("the_host_phasor");
const char * kMLPatcherProcName("voices/voice/patcher");

const float kMasterVolumeMaxRate = 5.f;

MLDSPEngine::MLDSPEngine() : 
mpInputToSignalsProc(0),
mpHostPhasorProc(0),
mInputChans(0),
mOutputChans(0),
mMasterVolume(1.0f),
mCollectStats(false),
mBufferSize(0),
mGraphStatus(unknownErr),
mCompileStatus(false),
mSamplesToProcess(0),
mStatsCount(0),
mSampleCount(0),
mCPUTimeCount(0.)
{
	setName("dspengine");
	setPropertyImmediate("demo", 1.f);
}

MLDSPEngine::~MLDSPEngine()
{
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
	
	juce::ScopedPointer<juce::XmlElement> pRootElem (pDoc->getDocumentElement());
	
	if (pRootElem)
	{	
		// sets root of this container to itself, which will be passed to children in buildGraph() / buildProc().
		setRootContext(this);
		
		// TODO what does this do different from the above, sort it out
		makeRoot("root");
		
		constexpr int kDefaultMaxVoices = 8;
		int maxVoices = pRootElem->getIntAttribute("max_voices", kDefaultMaxVoices);
		setMaxVoices(maxVoices);
	}
	
	// TODO refactor - paths to proccs are plugin-specific
	if (makeMidiInput)
	{
		// make XML node describing MIDI to signal processor.
		juce::ScopedPointer<juce::XmlElement> pElem (new juce::XmlElement("proc"));
		pElem->setAttribute("class", "midi_to_signals");
		pElem->setAttribute("name", juce::String(juce::CharPointer_UTF8(kMLInputToSignalProcName)));
		pElem->setAttribute("voices", getMaxVoices());			
		
		// build processor object.
		MLProc::err bpe = buildProc(pElem);
		
		// save a pointer to it.
		if (bpe == OK)
		{
			MLProcPtr pms = getProc(ml::Path(kMLInputToSignalProcName));
			if (pms)
			{
				mpInputToSignalsProc = static_cast<MLProcInputToSignals*>(&(*pms));
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
			MLProcPtr pms = getProc(ml::Path(kMLHostPhasorProcName));
			if (pms)
			{
				mpHostPhasorProc = static_cast<MLProcHostPhasor*>(&(*pms));
			}
		}
	}
	
	// make rest of graph
	if (pRootElem)
	{	
		buildGraph(pRootElem);
		
		// make any published signal outputs at top level only
		forEachXmlChildElement(*pRootElem, child)
		{ 
			if (child->hasTagName("signal"))
			{
				int mode = eMLRingBufferMostRecent;
				ml::Path procArg = RequiredPathAttribute(child, "proc");
				ml::Symbol outArg = RequiredAttribute(child, "output");
				ml::Symbol aliasArg = RequiredAttribute(child, "alias");
				
				if (procArg && outArg && aliasArg)
				{
					int bufLength = kMLRingBufferDefaultSize;
					bufLength = child->getIntAttribute("length", bufLength);
					int frameSize = 1;
					frameSize = child->getIntAttribute("frame_size", frameSize);
					ml::Path procPath (procArg);
					ml::Symbol outSym (outArg);
					ml::Symbol aliasSym (aliasArg);
					publishSignal(procPath, outSym, aliasSym, mode, bufLength, frameSize);
				}
			}
		}
		
		graphOK = true;
	}
	
	if (graphOK)
	{
		r = OK;
		mGraphStatus = OK;
	}
	
	return r;
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
		mCompileStatus = false;
	}  
	else
	{
		mCompileStatus = true;
	}
}

// prepareEngine() needs to be called if the sampling rate or block size changes.
//
MLProc::err MLDSPEngine::prepareEngine(double sr, int bufSize, int chunkSize)
{
	////debug() << " MLDSPEngine::prepareEngine: DSPEngine " << std::hex << (void *)this << std::dec << "\n";
	err e = OK;
	
	// set denormal state
	int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
	int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
	_mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
	// _mm_setcsr( oldMXCSR ); // restore MXCSR state (needed?)
	
	if ((mGraphStatus == OK) && (mCompileStatus))
	{
		// set self as context to get size and rate chain started.
		setContext(this);	
		
		int graphInputs = getNumInputs();
		
		// connect input Signals and set sizes.
		for (int i=0; i < graphInputs; ++i)
		{
			if(i < mInputChans)
			{
				mInputSignals[i]->setRate(sr);
				clearInput(i+1);
				e = setInput(i+1, (*mInputSignals[i]));
				if (e != OK) goto bail;
			}
			else
			{
				setInput(i+1, getContext()->getNullInput());				
			}
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
		
		int outs = mOutputChans;	
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
		int currSampleRate = getSampleRate();
		bool needsRebuild = (currSampleRate != sr);
		
		setSampleRate(sr);
		setBufferSize(bufSize);
		setVectorSize(chunkSize);
		
		if(needsRebuild)
		{
			// TODO rebuild
		}
		
		// after setVectorSize, set midiToSignals input buffer size.
		if (mpInputToSignalsProc)
		{
			////debug() << "MLDSPEngine::prepareEngine: bufsize: " << bufSize << ", vecSize: " << vecSize << "\n";
			mpInputToSignalsProc->setParam("bufsize", bufSize);
			mpInputToSignalsProc->resize();		
		}
		
		// setup volume filter
		mMasterVolumeFilter.setSampleRate(sr);
		mMasterVolumeFilter.setOnePole(kMasterVolumeMaxRate);
		mMasterVolumeSig.setDims(chunkSize);
		
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
		// TODO not MLSignals
		mInputSignals.push_back(MLSignalPtr(new MLSignal(kFloatsPerDSPVector)));
		
		mInputBuffers.push_back(std::unique_ptr<SignalBuffer>(new SignalBuffer()));
	}
	mNullInputSignal.setDims(kFloatsPerDSPVector);
	mNullInputSignal.clear();
}

void MLDSPEngine::setOutputChannels(int c) 
{
	mOutputChans = c;
	for (int i=0; i<mOutputChans; i++)
	{		
		mOutputBuffers.push_back(std::unique_ptr<SignalBuffer>(new SignalBuffer()));
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
		mInputBuffers[i]->read(mInputSignals[i]->getBuffer(), samples);
	}
}

void MLDSPEngine::multiplyOutputBuffersByVolume()
{
	int outs = mOutputChans;
	for(int i=0; i < outs; ++i)
	{
		MLSignal& output = getOutput(i+1);
		output.multiply(mMasterVolumeSig);
	}
} 

// write outputs of root container to ringbuffers
void MLDSPEngine::writeOutputBuffers(const int samples)
{
	int outs = mOutputChans;
	for(int i=0; i < outs; ++i)
	{
		mOutputBuffers[i]->write(getOutput(i+1).getBuffer(), samples);
	}
} 

// write outputs of root container to ringbuffers
void MLDSPEngine::clearOutputBuffers()
{
	int outs = mOutputChans;
	for(int i=0; i < outs; ++i)
	{
		mOutputBuffers[i]->clear();
	}
} 

// read ringbuffers to client output buffers
void MLDSPEngine::readOutputBuffers(const int samples)
{
	int outs = mOutputChans;
	int okToRead = true;
	for(int i=0; i < outs; ++i)
	{
		if(mOutputBuffers[i]->getReadAvailable() < samples)
		{
			okToRead = false;
			break;
		}
	}
	
	if(okToRead)
	{
		for(int i=0; i < outs; ++i)
		{
			mOutputBuffers[i]->read(mIOMap.outputs[i], samples);
		}
	}
	else
	{
		// clear outputs
		for(int i=0; i < outs; ++i)
		{
			// yuk, why do we have a raw float* here
			float * buf = mIOMap.outputs[i];
			for(int j=0; j<samples; ++j)
			{
				buf[j] = 0.f;
			}
		}		
	}
}

void MLDSPEngine::reset()
{
	if (mpInputToSignalsProc) // TODO inputToSignals need not be a kind of Proc
	{		
		mpInputToSignalsProc->clear();
	}
}

void MLDSPEngine::dump()
{
	dumpGraph(0);
}

// ----------------------------------------------------------------
#pragma mark published signals

void MLDSPEngine::publishSignal(const ml::Path & procAddress, const ml::Symbol outputName, const ml::Symbol alias,
																int trigMode, int bufLength, int frameSize)
{
	err e = addSignalBuffers(procAddress, outputName, alias, trigMode, bufLength, frameSize);
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
int MLDSPEngine::getPublishedSignalVoices(const ml::Symbol alias)
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
int MLDSPEngine::getPublishedSignalVoicesEnabled(const ml::Symbol alias)
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
	////debug() << "getPublishedSignalVoicesEnabled: " << alias << ": " << nVoices << "\n";
	return nVoices;
}

// get the buffer size for a published signal by looking at the length parameter
// of the first attached ring buffer.
//
int MLDSPEngine::getPublishedSignalBufferSize(const ml::Symbol alias)
{
	static const ml::Symbol lengthSym("length");
	
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
			result = proc->getParam(lengthSym);
		}
	}
	return result;
}

// read samples from a published signal list into outSig.
// return the number of samples read.
//
int MLDSPEngine::readPublishedSignal(const ml::Symbol alias, MLSignal& outSig)
{
	int nVoices = 0;
	int r;
	int minSamplesRead = 2<<16;
	
	outSig.clear();
	
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
					r = bufferProc.readToSignal(outSig, outSig.getWidth(), voice);
					minSamplesRead = ml::min(r, minSamplesRead);
					voice++;
				}
			}
		}
	}
#ifdef ML_DEBUG
	else
	{
		////debug() << "MLProcContainer::readPublishedSignal: signal " << alias << " not found in container " << getName() << "!\n";
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
void MLDSPEngine::setInputFrameBuffer(Queue<TouchFrame>* pBuf)
{
	if (mpInputToSignalsProc)
	{
		mpInputToSignalsProc->setInputFrameBuffer(pBuf);
	}
	else 
	{
		////debug() << "MLDSPEngine::setInputFrameBuffer: no mpInputToSignalsProc!\n";
	}
}

void MLDSPEngine::setMasterVolume(float v)
{
	mMasterVolume = v;
}

// ----------------------------------------------------------------
#pragma mark Process

void MLDSPEngine::setCollectStats(bool k)
{
	mCollectStats = k;
}

void MLDSPEngine::setTimeAndRate(const double secs, const double ppqPos, const double bpm, bool isPlaying)
{
	if (mpHostPhasorProc)
	{	
		mpHostPhasorProc->setTimeAndRate(secs, ppqPos, bpm, isPlaying);
	}	
}

// produce one signal vector of the compiled graph's output, processing signals from the global inputs (if any)
// to the global outputs. 
//
void MLDSPEngine::processDSPVector(Queue<MLControlEvent>& eventQueue, const uint64_t vectorStartTime)
{	
	readInputBuffers(kFloatsPerDSPVector);
	
	if (mpInputToSignalsProc) // TODO inputToSignals need not be a kind of Proc
	{						
		mpInputToSignalsProc->setVectorStartTime(vectorStartTime);
		
		// MLTEST this mpInputToSignalsProc Object / API will go away
		mpInputToSignalsProc->setQueue(&eventQueue);
		// when the queue can be a parameter in process(...) a reference can be used, not a pointer
	}
	
	// generate volume signal
	mMasterVolumeSig.fill(mMasterVolume);
	mMasterVolumeFilter.processSignalInPlace(mMasterVolumeSig);
	
	// MLProcContainer::process()
	process();  
	
	multiplyOutputBuffersByVolume();
	writeOutputBuffers(kFloatsPerDSPVector);
}




