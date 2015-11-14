
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginProcessor.h"

const int kMaxControlEventsPerBlock = 1024;
const char* kUDPAddressName = "localhost";

#if OSC_PARAMS	
	const int kSeqInfoPort = 9123;
	const int kUDPOutputBufferSize = 1024;
	const int kScribbleScenePort = 9223;
#endif

MLPluginProcessor::MLPluginProcessor() : 
	mInputProtocol(-1),
	mMLListener(0),
	mEditorNumbersOn(true),
	mEditorAnimationsOn(true),
	mInitialized(false)
{
	mHasParametersSet = false;
	mNumParameters = 0;
	lastPosInfo.resetToDefault();
    
    createFileCollections();
	
	// Files are scanned before UI is made in order to get MIDI programs, scales, etc.
    scanAllFilesImmediate();
    
    mControlEvents.reserve(kMaxControlEventsPerBlock);
	
	// initialize DSP state
	mpPatchState = std::unique_ptr<MLAppState>(new MLAppState(static_cast<MLModel*>(this),
		"patch", MLProjectInfo::makerName, MLProjectInfo::projectName, MLProjectInfo::versionNumber));
	
	// initialize environment model and state
	mpEnvironmentModel = std::unique_ptr<MLEnvironmentModel>(new MLEnvironmentModel(this));
	mpEnvironmentState = std::unique_ptr<MLAppState>(new MLAppState(mpEnvironmentModel.get(),
		"environment", MLProjectInfo::makerName, MLProjectInfo::projectName + std::string("Editor"), MLProjectInfo::versionNumber));
	
	// set up state to not save certain properties that are dynamic.
	mpPatchState->ignoreProperty("receiving_t3d");
	
#if defined (__APPLE__)
	// initialize T3D listener
	mT3DHub.addListener(this);
#endif
	
#if OSC_PARAMS	
	mpOSCBuf.resize(kUDPOutputBufferSize);
	mSeqInfoSocket = std::unique_ptr<UdpTransmitSocket>(new UdpTransmitSocket( IpEndpointName(kUDPAddressName, kSeqInfoPort)));		
	mVisSendCounter = 0;	
	
	mpOSCVisualsBuf.resize(kUDPOutputBufferSize);
	mVisualsSocket = std::unique_ptr<UdpTransmitSocket>(new UdpTransmitSocket( IpEndpointName(kUDPAddressName, kScribbleScenePort)));		
#endif		
}

MLPluginProcessor::~MLPluginProcessor()
{
#if defined (__APPLE__)
	mT3DHub.removeListener(this);
#endif
}

#pragma mark MLModel

// if a parameter of the DSP engine exists matching the property name, set it to the new value.
// TODO DSP engine can simply have properties.

void MLPluginProcessor::doPropertyChangeAction(MLSymbol propName, const MLProperty& newVal)
{
	int propertyType = newVal.getType();
	int paramIdx = getParameterIndex(propName);
	
	// debug() << "MLPluginProcessor::doPropertyChangeAction: " << propName << " (" << paramIdx << " of " << getNumParameters() << ") \n";
	
	if (!within(paramIdx, 0, getNumParameters())) 
	{
		return;
	}
	
	
	float f = newVal.getFloatValue();
	
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
		{
			// Here is where changes in Model properties turn into changes in DSP Engine parameters.
			MLPublishedParamPtr p = mEngine.getParamPtr(paramIdx);
			if(p)
			{
				// set published float parameter in DSP engine.
				setParameterWithoutProperty (propName, f);
				
				// convert to host units for VST
				f = newVal.getFloatValue();
				if (wrapperType == AudioProcessor::wrapperType_VST)
				{
					f = p->getValueAsLinearProportion();
				}
				
				// send change immediately to host wrapper
				if(within(paramIdx, 0, getNumParameters()))
				{
					AudioProcessor::sendParamChangeMessageToListeners (paramIdx, f);
				}
			}
		}
		break;
		case MLProperty::kStringProperty:
		{
			// set published string parameter in DSP engine.
			const std::string& stringVal = newVal.getStringValue();
			setStringParameterWithoutProperty (propName, stringVal);
		}
		break;
		case MLProperty::kSignalProperty:
		{
			// set published signal parameter in DSP engine.
			const MLSignal& sigVal = newVal.getSignalValue();
			setSignalParameterWithoutProperty (propName, sigVal);
		}
		break;
		default:
		break;
	}
}

void MLPluginProcessor::MLEnvironmentModel::doPropertyChangeAction(MLSymbol propName, const MLProperty& newVal)
{
	int propertyType = newVal.getType();
	
	// debug() << "Environment: " << propName << " to " << newVal << "\n";
	
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
		{
			// TODO these were getting reset to redundant values when the editor opens because it calls
			// updateAllProperties() on the environment. Listeners are where duplicate values get filtered out.
			// we want to update the new editor but not the t3d hub. so the editor and t3d hub should have separate
			// listeners from the environment properties. so maybe it's not a Model but just a PropertySet?
			// quick fix now, sort out later.
			if(propName == "protocol")
			{
				int p = newVal.getFloatValue();
				mpOwnerProcessor->setInputProtocol(p);
			}
#if ML_MAC
			else if(propName == "osc_port_offset")
			{
				int offset = newVal.getFloatValue();
				mpOwnerProcessor->mT3DHub.setShortName(MLProjectInfo::projectName);
				mpOwnerProcessor->mT3DHub.setPortOffset(offset);
			}
#endif
			break;
		}
		case MLProperty::kStringProperty:
			break;
		case MLProperty::kSignalProperty:
			break;
		default:
			break;
	}
}

void MLPluginProcessor::loadPluginDescription(const char* desc)
{
	mpPluginDoc = new XmlDocument(String(desc));
	
	if (mpPluginDoc.get())
	{
		ScopedPointer<XmlElement> doc (mpPluginDoc->getDocumentElement(true));
		if (doc)	// true = quick scan header
		{
			mEngine.scanDoc(&*mpPluginDoc, &mNumParameters);
			debug() << "loaded " << JucePlugin_Name << " plugin description, " << mNumParameters << " parameters.\n";
		}
		else
		{
			debug() << "MLPluginProcessor: error loading plugin description!\n";
		}   
	}
	else
	{
		debug() << "MLPluginProcessor: couldn't load plugin description!\n";
		return;
	}

	// build: turn XML description into graph of processors
	if (mEngine.getGraphStatus() != MLProc::OK)
	{
		int inChans = getNumInputChannels();
		bool makeSignalInputs = inChans > 0;
		mEngine.buildGraphAndInputs(&*mpPluginDoc, makeSignalInputs, wantsMIDI());
	}

	loadDefaultPreset();
	
	// get plugin parameters and initial values and create corresponding model properties.
	int params = getNumParameters();
	for(int i=0; i<params; ++i)
	{
		MLPublishedParamPtr p = getParameterPtr(i);
		MLPublishedParam* param = &(*p);
		if(param)
		{
			MLSymbol type = param->getType();
			if((type == "float") || (type == MLSymbol()))
			{
				setProperty(param->getAlias(), param->getDefault());
			}
		}
	}
}

void MLPluginProcessor::initializeProcessor()
{
	debug() <<  "initializing MLProcessor @ " << std::hex << (void*)this << std::dec << "...\n";
	
#if defined(__APPLE__)
	// connect t3d hub to DSP engine
	// send frame buf address to input proc.
	MLDSPEngine* pEngine = getEngine();
	if(pEngine)
	{
		pEngine->setInputFrameBuffer(mT3DHub.getFrameBuffer());
	}
	
	// publish t3d service and listen for incoming t3d data
	mT3DHub.setShortName(MLProjectInfo::projectName);
	mT3DHub.setPortOffset(mpEnvironmentModel->getFloatProperty("osc_port_offset"));
	
#endif
}

// editor creation function to be defined in (YourPluginEditor).cpp
//
extern juce::AudioProcessorEditor* CreateMLPluginEditor (MLPluginProcessor* const ownerProcessor);

juce::AudioProcessorEditor* MLPluginProcessor::createEditor()
{
	// creation function defined to return the plugin's flavor of plugin editor
    AudioProcessorEditor* r = CreateMLPluginEditor(this);
	
	return r;
}

void MLPluginProcessor::editorResized(int w, int h)
{
    mEditorRect.setWidth(w);
    mEditorRect.setHeight(h);
	MLSignal bounds(4);
	bounds.clear();
	bounds[2] = w;
	bounds[3] = h;
	mpEnvironmentModel->setProperty("editor_bounds", bounds);
}

#pragma mark preflight and cleanup
//

#ifdef ML_WINDOWS
static bool CheckSSE3()
{
    int CPUInfo[4] = {-1};
    
    //-- Get number of valid info ids
    __cpuid(CPUInfo, 0);
    int nIds = CPUInfo[0];
    
    //-- Get info for id "1"
    if (nIds >= 1)
    {
        __cpuid(CPUInfo, 1);
        bool bSSE3NewInstructions = (CPUInfo[2] & 0x1) || false;
        return bSSE3NewInstructions;
    }
    
    return false;
}
#endif

MLProc::err MLPluginProcessor::preflight(int requirements)
{
	MLProc::err e = MLProc::OK;
    switch(requirements)
    {
        default:
        case kRequiresSSE2:            
            if (!SystemStats::hasSSE2())
            {
                e = MLProc::SSE2RequiredErr;
            }
            break;
            
        case kRequiresSSE3:
#ifdef ML_WINDOWS
            // workaround for XP
            if (SystemStats::getOperatingSystemType() == SystemStats::WinXP)
            {
                bool sse3 = CheckSSE3();
                if(!sse3)
                {
                    e = MLProc::SSE3RequiredErr;
                }
            }
            else
#endif
            {
                if (!SystemStats::hasSSE3())
                {
                    e = MLProc::SSE3RequiredErr;
                }
            }
            break;
    }
	return e;
}

void MLPluginProcessor::prepareToPlay (double sr, int maxFramesPerBlock)
{
	MLProc::err prepareErr;
	MLProc::err r = preflight();
	
	if (!mpPluginDoc.get()) return;
	
	if (r == MLProc::OK)
	{
		// get the Juce process lock  // TODO ???
		const juce::ScopedLock sl (getCallbackLock());

		// make IO buffers
		mEngine.setInputChannels(getNumInputChannels());
		mEngine.setOutputChannels(getNumOutputChannels());

		int bufSize = 0;
		int chunkSize = 0;

		// choose new buffer size and vector size.
		{
			// bufSize is the smallest power of two greater than maxFramesPerBlock.
			int maxFramesBits = bitsToContain(maxFramesPerBlock);
			bufSize = 1 << maxFramesBits;
			
			// vector size is desired processing block size, set this to default size of signal.
			chunkSize = min((int)bufSize, (int)kMLProcessChunkSize);
		}	
		
		// dsp engine has one chunkSize of latency in order to run constant block size.
		setLatencySamples(chunkSize);

		debug() << "MLPluginProcessor: prepareToPlay: rate " << sr << ", buffer size " << bufSize << "\n";

#ifdef DEBUG
		theSymbolTable().audit();
		//theSymbolTable().dump();
#endif

		// compile: schedule graph of processors , setup connections, allocate buffers
		if (mEngine.getCompileStatus() != MLProc::OK)
		{
			mEngine.compileEngine();
		}
		else
		{
			debug() << "compile OK.\n";
		}

		// prepare to play: resize and clear processors
		prepareErr = mEngine.prepareEngine(sr, bufSize, chunkSize);
		if (prepareErr != MLProc::OK)
		{
			debug() << "MLPluginProcessor: prepareToPlay error: \n";
		}
		
		// after prepare to play, set state from saved blob if one exists
		const unsigned blobSize = mSavedBinaryState.getSize();
		if (blobSize > 0)
		{
			setPatchAndEnvStatesFromBinary (mSavedBinaryState.getData(), blobSize);
			mSavedBinaryState.setSize(0);
		}
		else 
		{
			mEngine.clear();
			if (!mHasParametersSet)
			{
				loadDefaultPreset();
			}
		}		
		
		// after setting state, initialize processor
		if(!mInitialized)
		{					
			initializeProcessor();
			mInitialized = true;
		}		
		
		mEngine.setEnabled(prepareErr == MLProc::OK);
		
		// mEngine.dump();		
	}
}

void MLPluginProcessor::reset()
{
	const juce::ScopedLock sl (getCallbackLock());
	mEngine.clear();
}

void MLPluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#pragma mark juce::AudioProcessor

void MLPluginProcessor::getStateInformation (MemoryBlock& destData)
{
	getPatchAndEnvStatesAsBinary(destData);
}

void MLPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// if uninitialized, save blob for later.
	if (mEngine.getCompileStatus() != MLProc::OK)
	{
		mSavedBinaryState.setSize(0);
		mSavedBinaryState.append(data, sizeInBytes);
	}
	else
	{
		setPatchAndEnvStatesFromBinary (data, sizeInBytes);
	}
}

// add the listener to all of our file collections.
void MLPluginProcessor::addFileCollectionListener(MLFileCollection::Listener* pL)
{
	mScaleFiles->addListener(pL);
	mPresetFiles->addListener(pL);
	mMIDIProgramFiles->addListener(pL);
}

#if ML_MAC
#pragma mark MLT3DHub::Listener

void MLPluginProcessor::handleHubNotification(MLSymbol action, const MLProperty prop)
{
	if(action == "receiving")
	{
		// set model property just for viewing. 
		int r = prop.getFloatValue();
		setProperty("receiving_t3d", r);
	}
	else if(action == "data_rate")
	{
		int r = prop.getFloatValue();
		if(r != (int)getFloatProperty("data_rate"))
		{
			setProperty("data_rate", r);
			getEngine()->setInputDataRate(r);
		}
	}
	else if(action == "program")
	{
		int r = prop.getFloatValue();
		loadPatchStateFromMIDIProgram(r);
#if OSC_PARAMS		
		sendProgramChange(r);
#endif
	}
	/*
	 TODO add to DSP engine
	else if(action == "volume")
	{
		getEngine()->setMasterVolume(prop.getFloatValue());
	}*/
	else if(action == "sequence")
	{
		setSequence(prop.getSignalValue());
	}
}

#endif

#pragma mark process

// Process any incoming MIDI messages, generating a vector of events for the DSP engine. 
// Some MIDI messages may also cause major side effects here like changing the program or input type (MPE) 
void MLPluginProcessor::processMIDI(MidiBuffer& midiMessages, MLControlEventVector& events)
{
 	bool addControlEvent;
    
	MidiBuffer::Iterator i (midiMessages);
    juce::MidiMessage message (0xf4, 0.0);
    MLControlEvent::EventType type = MLControlEvent::kNull;
    int chan = 0;
    int id = 0;
    int time = 0;
	double timeStamp;
    float v1 = 0.f;
    float v2 = 0.f;
		
	while (i.getNextEvent(message, time)) // writes to time
	{
		// default: most MIDI messages cause a control event to be made
		addControlEvent = true; 
		
        chan = message.getChannel();
		timeStamp = message.getTimeStamp();
		
		if (message.isNoteOn())
		{
            type = MLControlEvent::kNoteOn;
			v1 = message.getNoteNumber();
			v2 = message.getVelocity() / 127.f;
            id = (int)v1;
			//debug() << "NOTE ON: " << v1 << "\n";
		}
		else if(message.isNoteOff())
		{
            type = MLControlEvent::kNoteOff;
			v1 = message.getNoteNumber();
			v2 = message.getVelocity() / 127.f;
            id = (int)v1;// 
		}
		else if (message.isSustainPedalOn())
		{
            type = MLControlEvent::kSustainPedal;
			v1 = 1.f;
		}
		else if (message.isSustainPedalOff())
		{
            type = MLControlEvent::kSustainPedal;
			v1 = 0.f;
		}
		else if (message.isController())
		{
            type = MLControlEvent::kController;
			v1 = message.getControllerNumber();
			v2 = message.getControllerValue()/127.f;
			
			// debug() << "CONTROLLER # " << v1 << " : " << message.getControllerValue() << "\n";
			
			/*
			// look for MPE Mode messages
			if(v1 == 127)
			{
				// MPE switch, nothing to do in engine.
				addControlEvent = false; 
				int chans = clamp(message.getControllerValue(), 0, 15);	
				
				if(chans > 0)
				{
					getEnvironment()->setPropertyImmediate("protocol", kInputProtocolMIDI_MPE);			
				}
			}
			*/
		}
		else if (message.isPitchWheel())
		{
            type = MLControlEvent::kPitchWheel;
			v1 = message.getPitchWheelValue();
		}
		else if (message.isAftertouch())
		{
			type = MLControlEvent::kNotePressure;
			v1 = message.getNoteNumber();
			v2 = message.getAfterTouchValue() / 127.f;
            id = (int)v1;
		}
		else if (message.isChannelPressure())
		{
            type = MLControlEvent::kChannelPressure;
			v1 = message.getChannelPressureValue() / 127.f;
		}
		else if (message.isProgramChange())
		{
			int pgm = message.getProgramChangeNumber();
			if(pgm == kMLPluginMIDIPrograms)
			{
				// load most recent saved program
				returnToLatestStateLoaded();
			}
			else
			{		
				loadPatchStateFromMIDIProgram(pgm);
			}
			// currently unused. but a sample-accurate program change might be
			// useful in the future, especially for offline rendering.
            type = MLControlEvent::kProgramChange;
            id = chan;
            v1 = (float)pgm;
		}
	
		/*
		 else if (!message.isMidiClock())
		// TEST
		{
			int msgSize = message.getRawDataSize();
			const uint8* msgData = message.getRawData();
			debug() << "@" << std::hex << (void*)this << ": " << msgSize << "bytes uncaught MIDI [" ;
			
			for(int b=0; b<message.getRawDataSize(); ++b)
			{
				debug() << std::hex << (unsigned int)(msgData[b]) << " ";
			}	
			debug() << std::dec << "]\n";
		}
		 */
        if(addControlEvent && (events.size() < kMaxControlEventsPerBlock))
        {
            events.push_back(MLControlEvent(type, chan, id, time, v1, v2));
        }
	}
    
    // null-terminate new event list
    events.push_back(kMLNullControlEvent);
}

void MLPluginProcessor::setCollectStats(bool k)
{
	mEngine.setCollectStats(k);
}

void MLPluginProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	if (mEngine.isEnabled() && !isSuspended())
	{
		unsigned samples = buffer.getNumSamples();
		
		// get current time from host.
		// should refer to the start of the current block.
		AudioPlayHead::CurrentPositionInfo newTime;
		if (getPlayHead() != 0 && getPlayHead()->getCurrentPosition (newTime))
		{
			lastPosInfo = newTime;
		}
		else
		{
			lastPosInfo.resetToDefault();
		}
		
		// set host phasor
		double bpm = lastPosInfo.isPlaying ? lastPosInfo.bpm : 0.;
		double ppqPosition = lastPosInfo.ppqPosition;
		double secsPosition = lastPosInfo.timeInSeconds;
		int64 samplesPosition = lastPosInfo.timeInSamples;
		bool isPlaying = lastPosInfo.isPlaying;
			
		// set Engine I/O.  done here each time because JUCE may change pointers on us.  possibly.
		MLDSPEngine::ClientIOMap ioMap;
		for (int i=0; i<getNumInputChannels(); ++i)
		{
			ioMap.inputs[i] = buffer.getReadPointer(i);
		}		
		for (int i=0; i<getNumOutputChannels(); ++i)
		{
			ioMap.outputs[i] = buffer.getWritePointer(i);
		}
		mEngine.setIOBuffers(ioMap);
        
        if(acceptsMidi())
        {
			mControlEvents.clear();
            processMIDI(midiMessages, mControlEvents);
            midiMessages.clear(); // otherwise messages will be passed back to the host
        }
		
        mEngine.processSignalsAndEvents(samples, mControlEvents, samplesPosition, secsPosition, ppqPosition, bpm, isPlaying);

		
#if OSC_PARAMS
		// if(osc enabled)
		mVisSendCounter += samples;
		int samplesPerSecond = 44100;
		int period = samplesPerSecond/30;
		if(mVisSendCounter > period)
		{
			sendSeqInfo();
			sendVisuals();
			mVisSendCounter -= period;
		}
#endif		
    }
	else
	{
		buffer.clear();
	}
}

#pragma mark parameters
//

int MLPluginProcessor::getNumParameters() 
{
	return mNumParameters;
}

int MLPluginProcessor::getParameterIndex (const MLSymbol name)
{
 	return mEngine.getParamIndex(name);
}

float MLPluginProcessor::getParameter (int index)
{
	if (!within(index, 0, getNumParameters())) return(0);
	return mEngine.getParamByIndex(index);
}

// set scalar float plugin parameter by index. Typically called by the host wrapper.
// The Property must propagate to other Listeners, but not back to this MLPluginProcessor object.
//
void MLPluginProcessor::setParameter (int index, float newValue)
{
	if (!within(index, 0, getNumParameters())) return;
	mEngine.setPublishedParam(index, MLProperty(newValue));
	mHasParametersSet = true;
	
	// exclude this listener to avoid feedback!
	setPropertyImmediateExcludingListener(getParameterAlias(index), newValue, this);
}

// for VST wrapper.
float MLPluginProcessor::getParameterAsLinearProportion (int index)
{
	float r = 0;
  	if (!within(index, 0, getNumParameters())) 
	{
		// MLTEST
		debug() << "WARNING: param " << index << " does not exist!\n";
		return(0);
	}
	MLPublishedParamPtr p = mEngine.getParamPtr(index);
	if(p)
	{	
		r = p->getValueAsLinearProportion();
	}
	return r;
}

// for VST wrapper.
void MLPluginProcessor::setParameterAsLinearProportion (int index, float newValue)
{
	if (!within(index, 0, getNumParameters())) return;
	MLPublishedParamPtr p = mEngine.getParamPtr(index);
	if(p)
	{
		p->setValueAsLinearProportion(newValue);	
		mEngine.setPublishedParam(index, MLProperty(p->getValue()));
		mHasParametersSet = true;
		
		// set MLModel Parameter 
		// exclude this listener to avoid feedback!
		MLSymbol paramName = getParameterAlias(index);
		float realVal = mEngine.getParamByIndex(index);
		setPropertyImmediateExcludingListener(paramName, realVal, this);
	}
}

float MLPluginProcessor::getParameterMin (int index)
{
	if (!within(index, 0, getNumParameters())) return(0);
	return mEngine.getParamPtr(index)->getRangeLo();
}

float MLPluginProcessor::getParameterMax (int index)
{
	if (!within(index, 0, getNumParameters())) return(0);
	return mEngine.getParamPtr(index)->getRangeHi();
}

const String MLPluginProcessor::getParameterName (int index)
{
	if (!within(index, 0, getNumParameters())) return String();
	MLSymbol nameSym;
	const int p = mEngine.getPublishedParams(); 
	
	if (p == 0) // doc has been scanned but not built
	{
		nameSym = MLSymbol("param").withFinalNumber(index);
	}
	else
	{
		// graph has been built
		nameSym = mEngine.getParamPtr(index)->getAlias();	
		//debug() << "getParameterName: " << index << " is " << nameSym.getString().c_str() << ".\n";
	}
	
	return (String(nameSym.getString().c_str()));
}

const String MLPluginProcessor::symbolToXMLAttr(const MLSymbol sym)
{
	std::string nameCopy = sym.getString();
	
	unsigned len = nameCopy.length();
	for(unsigned c = 0; c < len; ++c)
	{
		unsigned char k = nameCopy[c];
		if(k == '#')
		{
			nameCopy[c] = ':';
		}
		else if(k == '*')
		{
			nameCopy[c] = 0xB7;
		}
	}
 	return (String(nameCopy.c_str()));
}

const MLSymbol MLPluginProcessor::XMLAttrToSymbol(const String& str)
{
	std::string strCopy(str.toUTF8());
	
	unsigned len = strCopy.length();
	for(unsigned c = 0; c < len; ++c)
	{
		unsigned char k = strCopy[c];
		if(k == ':')
		{
			strCopy[c] = '#';
		}
		else if(k == 0xB7)
		{
			strCopy[c] = '*';
		}
	}
 	return (MLSymbol(strCopy.c_str()));
}

const MLSymbol MLPluginProcessor::getParameterAlias (int index)
{
	if (!within(index, 0, getNumParameters())) return MLSymbol();
 	return mEngine.getParamPtr(index)->getAlias();
}

float MLPluginProcessor::getParameterDefaultValue (int index)
{
	if (!within(index, 0, getNumParameters())) return 0.;
 	return mEngine.getParamPtr(index)->getDefault();
}

MLPublishedParamPtr MLPluginProcessor::getParameterPtr (int index)
{
 	return mEngine.getParamPtr(index);
}

MLPublishedParamPtr MLPluginProcessor::getParameterPtr (MLSymbol sym)
{
 	return mEngine.getParamPtr(mEngine.getParamIndex(sym));
}

const String MLPluginProcessor::getParameterText (int index)
{
	// get -inf and indexed values here?
	float val = mEngine.getParamByIndex(index);
    return String (val, 2);
}

const std::string& MLPluginProcessor::getParameterGroupName (int index)
{
	return mEngine.getParamGroupName(index);
}

bool MLPluginProcessor::isParameterAutomatable (int idx) const 
{
	if (!within(idx, 0, mNumParameters)) return false;
	return mEngine.getParamPtr(idx)->getAutomatable();
}

// set scalar float plugin parameter by name without setting property.
//
void MLPluginProcessor::setParameterWithoutProperty (MLSymbol paramName, float newValue)
{
	int index = getParameterIndex(paramName);
	if (!within(index, 0, getNumParameters())) return;
	
	mEngine.setPublishedParam(index, MLProperty(newValue));
	mHasParametersSet = true;
}

// set string plugin parameter by name without setting property.
//
void MLPluginProcessor::setStringParameterWithoutProperty (MLSymbol paramName, const std::string& newValue)
{
	int index = getParameterIndex(paramName);
	if (!within(index, 0, getNumParameters())) return;	
	mEngine.setPublishedParam(index, MLProperty(newValue));
	mHasParametersSet = true;
}

// set signal plugin parameter by name without setting property.
//
void MLPluginProcessor::setSignalParameterWithoutProperty (MLSymbol paramName, const MLSignal& newValue)
{
	int index = getParameterIndex(paramName);
	if (!within(index, 0, getNumParameters())) return;
	
	mEngine.setPublishedParam(index, MLProperty(newValue));
	mHasParametersSet = true;
}

// count the number of published copies of the signal matching alias.
int MLPluginProcessor::countSignals(const MLSymbol alias)
{
	int numSignals = mEngine.getPublishedSignalVoicesEnabled(alias);
	return numSignals;
}

#pragma mark get and save state

// auto-increment a version number and save at the current preset location with the new pathname.
int MLPluginProcessor::saveStateAsVersion()
{
    int r = 0;
	int version = 0;
    std::string nameStr(getStringProperty("preset"));
	std::string noVersionStr;
	std::string versionStr;
	int numberStart = 0;
	int digits = 0;
	
	// get version number
	int size = nameStr.size();
	char c = nameStr[size - 1];
	if(c == ']')
	{
		numberStart = size - 2;
		while((numberStart > 0) && isdigit(nameStr[numberStart]))
		{
			numberStart--;
			digits++;
		}
		numberStart++;
		
		noVersionStr = nameStr.substr(0, numberStart - 1);
		versionStr = nameStr.substr(numberStart, digits);
		version = atoi(versionStr.c_str());
	}
	else
	{
		noVersionStr = nameStr;
		version = 0;
	}
	
	version++;
	version = clamp(version, 1, 9999);
	char vBuf[16];
	sprintf(vBuf, "[%d]", version);
    std::string newName = noVersionStr + vBuf;
    saveStateToRelativePath(newName);
    r = 0;
	return r;
}

int MLPluginProcessor::saveStateOverPrevious()
{
    saveStateToRelativePath(getStringProperty("preset"));
	return 0;
}

// save with long file name as input -- needed for using system File dialogs
// return nonzero on error.
//
void MLPluginProcessor::saveStateToLongFileName(const std::string& longName)
{
    int r = true;
    std::string newPath = mPresetFiles->getRelativePathFromName(longName);
    if(newPath != "")
    {
        saveStateToRelativePath(newPath);
    }
    else
    {
		// warn that path is outside the usual directory.
		String errStr = "Note: the location ";
		errStr += longName;
		errStr += " is outside of the ";
		errStr += MLProjectInfo::projectName;
		errStr += " folder. The saved file will not appear in the preset menu. Save anyway?";
		if(! AlertWindow::showOkCancelBox (AlertWindow::NoIcon, String::empty, errStr, "OK", "Cancel")) return;
		
		// use only the short name as model param.
		std::string shortName = ml::stringUtils::getShortName(longName);
		setProperty("preset", shortName);
		
		std::string extension (".mlpreset");
		std::string newFilePath = longName + extension;
		
		// using juce files, TODO revisit
		String newFileName(newFilePath.c_str());
		juce::File newFile(newFileName);
		if(!(newFile.exists()))
		{
			r = newFile.replaceWithText(getStateAsText());
		}
		else
		{
			String errStr = "The file ";
			errStr += longName;
			errStr += " already exists. Overwrite? ";
			if(! AlertWindow::showOkCancelBox (AlertWindow::NoIcon, String::empty, errStr, "OK", "Cancel")) return;
			r = newFile.replaceWithText(getStateAsText());
		}
		
		if(r != true)
		{
			String errStr = "Error saving file to ";
			errStr += longName;
			AlertWindow::showMessageBox (AlertWindow::NoIcon, String::empty, errStr, "OK");
		}
		else
		{
			// reset state stack and push current state for recall
			mpPatchState->clearStateStack();
			mpPatchState->pushStateToStack();
		}
    }
    return;
}

// creates a file with the right extension for the plugin type.
// as of Aalto 1.6 / Kaivo this is always .mlpreset.
// input: a file path relative to the presets root, without extension.
//
void MLPluginProcessor::saveStateToRelativePath(const std::string& path)
{
#if DEMO
	debug() << "DEMO version. Saving is disabled.\n";
#else
    
    // the Model param contains the file path relative to the root.
    setProperty("preset", path);
	
    std::string extension (".mlpreset");
    std::string extPath = path + extension;
    const MLFile& f = mPresetFiles->createFile(extPath);
    if(!f.getJuceFile().exists())
    {
        f.getJuceFile().create();
    }
	
    f.getJuceFile().replaceWithText(getStateAsText());
	
	// reset state stack and push current state for recall
	mpPatchState->clearStateStack();
	mpPatchState->pushStateToStack();
	
#endif // DEMO
    
}

String MLPluginProcessor::getStateAsText()
{
	String r;
	if(mpPatchState)
	{
		r = mpPatchState->getStateAsText();
	}
	return r;
}

void MLPluginProcessor::getPatchAndEnvStatesAsBinary (MemoryBlock& destData)
{
	// get processor state as JSON
	cJSON* procRoot = mpPatchState->getStateAsJSON();
	
	// get environment state as JSON
	cJSON* envRoot = mpEnvironmentState->getStateAsJSON();
	
	// combine the states
	cJSON* combinedRoot = cJSON_CreateObject();
	
	std::string combinedStateStr;
	cJSON_AddItemToObject(combinedRoot, "patch", procRoot);
	cJSON_AddItemToObject(combinedRoot, "environment", envRoot);
	{
		char * stateText = cJSON_Print(combinedRoot);
		combinedStateStr = stateText;
		free(stateText);
	}
	
	if(combinedStateStr.length() > 0)
	{
		// TODO compress here
		int stateStrLen = combinedStateStr.size();
		destData.replaceWith(combinedStateStr.data(), stateStrLen);
	}

	cJSON_Delete(combinedRoot); // also deletes procRoot and envRoot
}

#pragma mark load state from file

void MLPluginProcessor::loadStateFromPath(const std::string& path)
{
    if(path != std::string())
    {
        const MLFile& f = mPresetFiles->getFileByPath(path);
        if(f.exists())
        {
            loadPatchStateFromFile(f);
            std::string shortPath = ml::stringUtils::stripExtension(path);
            setProperty("preset", shortPath);
        }
    }
}

void MLPluginProcessor::loadPatchStateFromFile(const MLFile& f)
{
	if (f.exists())
	{
		String extension = f.getJuceFile().getFileExtension();
		if (extension == ".mlpreset")
		{
			// .mlpreset files may be XML (old) or JSON (new)
			setPatchStateFromText(f.getJuceFile().loadFileAsString());
		}
#if ML_MAC
		else if (extension == ".aupreset")
		{
			// .aupreset files are XML inside Mac OS Property File wrapper
			std::string pluginType;
			switch(wrapperType)
			{
				case AudioProcessor::wrapperType_VST:
					{
						pluginType = "VST";
						juce::ScopedPointer<juce::XmlElement> pXML = loadPropertyFileToXML(f.getJuceFile());
						if(pXML)
						{
							juce::MemoryBlock destData;
							MemoryOutputStream out (destData, false);
							pXML->writeToStream (out, String(), true, false);
							String xmlStr = out.toUTF8();
							setPatchStateFromText(xmlStr);
						}
					}
					break;
				case AudioProcessor::wrapperType_AudioUnit:
					pluginType = "AU";
					
					// tell AU wrapper to load AU-compatible .aupreset file.
					sendMessageToMLListener (MLAudioProcessorListener::kLoad, f.getJuceFile());

					break;
				case AudioProcessor::wrapperType_Standalone:
					pluginType = "App";
					break;
				default:
					pluginType = "?";
					break;
			}
		}
#endif
		// replace app state with new state loaded
		mpPatchState->updateChangedProperties();
		mpPatchState->clearStateStack();
		mpPatchState->pushStateToStack();
	}
}

void MLPluginProcessor::returnToLatestStateLoaded()
{
	mpPatchState->returnToFirstSavedState();
	updateAllProperties();
}

#pragma mark set state

// set Processor and Environment states from XML or JSON in binary.
// state is in JSON format. XML can be read for backwards compatibility.
//
void MLPluginProcessor::setPatchAndEnvStatesFromBinary (const void* data, int sizeInBytes)
{
	// first try getting XML from binary- this will fail if blob is not in XML format
	XmlElementPtr xmlState(getXmlFromBinary (data, sizeInBytes));
	if (xmlState)
	{
		bool setViewAttributes = true;
		setStateFromXML(*xmlState, setViewAttributes);
		updateChangedProperties();
	}
	else
	{
		// TODO uncompress here
		std::string stateStr (static_cast<const char *>(data), sizeInBytes);
		
		// trim starting whitespace
		const char * pStart = stateStr.data();
		const char * pTrimmedStart = pStart;
		while(isspace(*pTrimmedStart) && (pTrimmedStart - pStart < sizeInBytes))
		{
			pTrimmedStart++;
		}
		
		// assume JSON
		bool OK = true;
		
		// debug() << "STATES:\n" << pTrimmedStart << "\n";
		cJSON* root = cJSON_Parse(pTrimmedStart);
		if(root)
		{
			cJSON* patchState = cJSON_GetObjectItem(root, "patch");
			if(patchState)
			{
				mpPatchState->setStateFromJSON(patchState);
				// TODO updateAllProperties is needed to make restore state work after MIDI parameter changes,
				// as opposed to parameter changes from UI. updateChangedProperties should be made to work instead.
				updateAllProperties();
			}
			else
			{
				OK = false;
			}
			
			cJSON* environmentState = cJSON_GetObjectItem(root, "environment");
			if(environmentState)
			{
				mpEnvironmentState->setStateFromJSON(environmentState);
			}
			else
			{
				OK = false;
			}
			
			cJSON_Delete(root);
		}
		
		if(!OK)
		{
			// TODO notify user in release
			debug() << "MLPluginProcessor::setPatchAndEnvStatesFromBinary: couldn't load JSON!\n";
		}
	}
	
	// push state for access by "Revert To saved"
	mpPatchState->clearStateStack();
	mpPatchState->pushStateToStack();
}

void MLPluginProcessor::loadPatchStateFromMIDIProgram (const int idx)
{
	int pgm = clamp(idx, 0, kMLPluginMIDIPrograms - 1);
	loadPatchStateFromFile(mMIDIProgramFiles->getFileByIndex(pgm));
}

void MLPluginProcessor::setPatchStateFromText (const String& stateStr)
{
	const String& trimmedState = stateStr.trimStart();
	if(trimmedState[0] == '{')
	{
		// assume JSON
		cJSON* root = cJSON_Parse(trimmedState.toUTF8());
		if(root)
		{
			mpPatchState->setStateFromJSON(root);
			// TODO updateAllProperties is needed to make restore state work after MIDI parameter changes,
			// as opposed to parameter changes from UI. updateChangedProperties should be made to work instead.
			updateAllProperties();
			cJSON_Delete(root);
		}
		else
		{
			debug() << "MLPluginProcessor::setPatchStateFromText: couldn't create JSON object!\n";
		}
	}
	else if (trimmedState[0] == '<')
	{
		// assume XML
		ScopedPointer<XmlDocument> stateToLoad (new XmlDocument(trimmedState));
		if (stateToLoad != NULL)
		{
			XmlElementPtr pDocElem (stateToLoad->getDocumentElement(true));
			if(pDocElem)
			{
				setStateFromXML(*pDocElem, false);
			}
			else
			{
				debug() << "ERROR parsing patch: " << stateToLoad->getLastParseError() << "\n";
			}
		}
		else
		{
			debug() << "MLPluginProcessor::setPatchStateFromText: couldn't create XML object!\n";
		}
	}
	else
	{
		debug() << "MLPluginProcessor::setPatchStateFromText: unknown format for .mlpreset file!\n";
	}
	
	// update all properties from previous state of Model. This will cause DSP parameter changes
	// to happen through doPropertyChangeAction().
	updateAllProperties();
}

void MLPluginProcessor::setStateFromXML(const XmlElement& xmlState, bool setViewAttributes)
{
	if (!(xmlState.hasTagName (JucePlugin_Name))) return;
	if (!(mEngine.getCompileStatus() == MLProc::OK)) return; // TODO revisit need to compile first
	
	// only the differences between default parameters and the program state are saved in an XML program,
	// so the first step is to set the default parameters.
	setDefaultParameters();
	
	// get program version of saved state
	unsigned blobVersion = xmlState.getIntAttribute ("pluginVersion");
	unsigned pluginVersion = JucePlugin_VersionCode;
	
	if (blobVersion > pluginVersion)
	{
		// TODO show error to user
		debug() << "MLPluginProcessor::setStateFromXML: saved program version is newer than plugin version!\n";
		return;
	}
    
	// try to load scale if a scale attribute exists
    // TODO auto save all state including this
 	const String scaleDir = xmlState.getStringAttribute ("scaleDir"); // look for old-style dir attribute
	const String scaleName = xmlState.getStringAttribute ("scaleName");
    String fullName;
    if(scaleName != String::empty)
    {
        fullName = scaleName;
        if(scaleDir != String::empty)
        {
            fullName = scaleDir + String("/") + fullName + ".scl";
        }
    }
    else
    {
        fullName = "12-equal";
    }
    std::string fullScaleName(fullName.toUTF8());
	setProperty("key_scale", fullScaleName);
	
	// get preset name saved in blob.  when saving from AU host, name will also be set from RestoreState().
	const String presetName = xmlState.getStringAttribute ("presetName");
	setProperty("preset", std::string(presetName.toUTF8()));
    
	/*
     debug() << "MLPluginProcessor: setStateFromXML: loading program " << presetName << ", version " << std::hex << blobVersion << std::dec << "\n";
     MemoryOutputStream myStream;
     xmlState->writeToStream (myStream, "");
     debug() << myStream.toString();
     */
	
	// get plugin-specific translation table for updating older versions of data
	std::map<MLSymbol, MLSymbol> translationTable;
	
	// TODO move this into Aalto!
	// make translation tables based on program version.
	//
	if (blobVersion <= 0x00010120)
	{
		// translate seq parameters
		for(unsigned n=0; n<16; ++n)
		{
			std::stringstream pName;
			std::stringstream pName2;
			pName << "seq_value" << n;
			pName2 << "seq_pulse" << n;
			MLSymbol oldSym(pName.str());
			MLSymbol newSym = MLSymbol("seq_value#").withFinalNumber(n);
			MLSymbol oldSym2(pName2.str());
			MLSymbol newSym2 = MLSymbol("seq_pulse#").withFinalNumber(n);
			translationTable[oldSym] = newSym;
			translationTable[oldSym2] = newSym2;
		}
	}
	
	if (blobVersion <= 0x00010200)
	{
		MLSymbol oldSym = MLSymbol("seq_value");
		MLSymbol newSym = MLSymbol("seq_value").withFinalNumber(0);
		MLSymbol oldSym2 = MLSymbol("seq_pulse");
		MLSymbol newSym2 = MLSymbol("seq_pulse").withFinalNumber(0);
		translationTable[oldSym] = newSym;
		translationTable[oldSym2] = newSym2;
		
		// translate seq parameters
		for(unsigned n=1; n<16; ++n)
		{
			oldSym = MLSymbol("seq_value#").withFinalNumber(n);
			newSym = MLSymbol("seq_value").withFinalNumber(n);
			oldSym2 = MLSymbol("seq_pulse#").withFinalNumber(n);
			newSym2 = MLSymbol("seq_pulse").withFinalNumber(n);
			translationTable[oldSym] = newSym;
			translationTable[oldSym2] = newSym2;
		}
	}
	
	// get patcher matrix from old-style input params
	String patcherInputStr ("patcher_input_");
	
	// get params from xml
	const unsigned numAttrs = xmlState.getNumAttributes();
	
	for(unsigned i=0; i<numAttrs; ++i)
	{
		// get name / value pair.
		const String& attrName = xmlState.getAttributeName(i);
		const MLParamValue paramVal = xmlState.getDoubleAttribute(attrName);
		
		// if not a patcher input setting,
		if (!attrName.contains(patcherInputStr))
		{
			// see if we have this named parameter in our engine.
			MLSymbol paramSym = XMLAttrToSymbol(attrName);
			const int pIdx = getParameterIndex(paramSym);
			
			if (pIdx >= 0)
			{
				// debug() << "setStateFromXML: <" << paramSym << " = " << paramVal << ">\n";
				setProperty(paramSym, paramVal);
			}
			else // try finding a match through translation table.
			{
				//debug() << "Looking for parameter " << paramSym << " in table...\n";
				std::map<MLSymbol, MLSymbol>::iterator it;
				it = translationTable.find(paramSym);
				if (it != translationTable.end())
				{
					const MLSymbol newSym = translationTable[paramSym];
					const int pNewIdx = getParameterIndex(newSym);
					if (pNewIdx >= 0)
					{
						//debug() << "translated parameter to " << newSym << " .\n";
						setProperty(newSym, paramVal);
					}
					else
					{
						debug() << "MLPluginProcessor::setStateFromXML: no such parameter! \n";
					}
				}
				else
				{
					// fail silently on unfound params, because we have deprecated some but they may still
					// be around in old presets.
					//debug() << "MLPluginProcessor::setStateFromXML: parameter " << paramSym << " not found!\n";
				}
			}
		}
	}
	
	// get editor state from XML
    if(setViewAttributes)
	{
		int x = xmlState.getIntAttribute("editor_x");
		int y = xmlState.getIntAttribute("editor_y");
		int width = xmlState.getIntAttribute("editor_width");
		int height = xmlState.getIntAttribute("editor_height");
		mEditorRect = MLRect(x, y, width, height);
		mEditorNumbersOn = xmlState.getIntAttribute("editor_num", 1);
		mEditorAnimationsOn = xmlState.getIntAttribute("editor_anim", 1);
	}
}

#pragma mark MIDI programs

void MLPluginProcessor::createFileCollections()
{
	mScaleFiles = (std::unique_ptr<MLFileCollection>)(new MLFileCollection("scales", getDefaultFileLocation(kScaleFiles), "scl"));
    mPresetFiles = (std::unique_ptr<MLFileCollection>)(new MLFileCollection("presets", getDefaultFileLocation(kPresetFiles), "mlpreset"));
    mMIDIProgramFiles = (std::unique_ptr<MLFileCollection>)(new MLFileCollection("midi_programs", getDefaultFileLocation(kPresetFiles).getChildFile("MIDI Programs"), "mlpreset"));
}

void MLPluginProcessor::scanAllFilesImmediate()
{
    mScaleFiles->processFilesImmediate();
    mPresetFiles->processFilesImmediate();
    mMIDIProgramFiles->processFilesImmediate();
}

#pragma mark presets


void MLPluginProcessor::prevPreset()
{
    advancePreset(-1);
}

void MLPluginProcessor::nextPreset()
{
    advancePreset(1);
}

void MLPluginProcessor::advancePreset(int amount)
{
    int len = mPresetFiles->getSize();
    int currIdx = mPresetFiles->getFileIndexByPath(getStringProperty("preset"));
    
    if(currIdx >= 0)
    {
        currIdx += amount;
    }
    else
    {
        // not found
        currIdx = 0;
    }
    if(currIdx < 0)
    {
        currIdx = len - 1;
    }
    if(currIdx >= len)
    {
        currIdx = 0;
    }
	
    std::string relPath = mPresetFiles->getFilePathByIndex(currIdx);
    loadStateFromPath(relPath);
}

void MLPluginProcessor::setDefaultParameters()
{
	if (mEngine.getCompileStatus() == MLProc::OK)
	{
		// set default for each parameter.
		const unsigned numParams = getNumParameters();
		for(unsigned i=0; i<numParams; ++i)
		{
			MLPublishedParamPtr paramPtr = getParameterPtr(i);
			MLSymbol paramType = paramPtr->getType();
			if(paramType == "float")
			{
				float defaultVal = getParameterDefaultValue(i);
				setPropertyImmediate(getParameterAlias(i), defaultVal);
			}
			else if (paramType == "string")
			{
				// unimplemented
			}
			else if (paramType == "signal")
			{
				const MLProperty& p = getProperty(getParameterAlias(i));
				if (p.getType() == MLProperty::kSignalProperty)
				{
					// TODO set up defaults for signal params once we are loading from JSON
					// right now we clear to zero
					MLSignal defaultSignal(p.getSignalValue());
					defaultSignal.clear();
					setPropertyImmediate(getParameterAlias(i), defaultSignal);
				}
			}
		}
	}
}

#pragma mark channels

const String MLPluginProcessor::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String MLPluginProcessor::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool MLPluginProcessor::isInputChannelStereoPair (int ) const
{
    return true;
}

bool MLPluginProcessor::isOutputChannelStereoPair (int ) const
{
    return true;
}

double MLPluginProcessor::getTailLengthSeconds() const
{
	return 0.f;
}

bool MLPluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MLPluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

void MLPluginProcessor::setMLListener (MLAudioProcessorListener* const newListener) throw()
{
	assert(newListener);
    mMLListener = newListener;
}

MLProc::err MLPluginProcessor::sendMessageToMLListener (unsigned msg, const File& f)
{
	MLProc::err err = MLProc::OK;
	if(!mMLListener) return MLProc::unknownErr;

	switch(msg)
	{
		case MLAudioProcessorListener::kLoad:
			mMLListener->loadFile (f);
		break;
		case MLAudioProcessorListener::kSave:
			mMLListener->saveToFile (f);
		break;
		default:
		break;
	}
	return err;
}

// called to change the input protocol.
//
void MLPluginProcessor::setInputProtocol(int p)
{
	if(p != mInputProtocol)
	{
		getEngine()->setEngineInputProtocol(p);
		
#if ML_MAC
		if(p == kInputProtocolOSC)
		{
			mT3DHub.setEnabled(true);
		}
		else
		{
			mT3DHub.setEnabled(false);
		}		
#endif
		mInputProtocol = p;
	}
}

#if OSC_PARAMS
#include "MLProcStepSequencer.h"
const char * kMLStepSeqProcName("voices/voice/seq/seq");

void MLPluginProcessor::sendSeqInfo()
{
	int chan = mT3DHub.getPortOffset();
	//int seqData = mT3DHub.getPortOffset();
	
	MLDSPEngine* eng = getEngine();
	
	// if we made one or more Patchers with the right names in the document, save a list of them for direct access. 
	eng->getProcList(mSequencerList, MLPath(kMLStepSeqProcName), kMLEngineMaxVoices);
	// debug() << "got " << mSequencerList.size() << "seqs\n";
	//int nSeqs = mSequencerList.size();
	
	
	/* TODO make ProcList a vector
	 for(int i=0; i<nSeqs; ++i)
	 {
		int step = mSequencerList.begin().getStep();
	 }
	 */
	
	int32_t brightLights = 0;
	int32_t dimLights = 0;
	for (MLProcList::const_iterator jt = mSequencerList.begin(); jt != mSequencerList.end(); jt++)
	{
		MLProcPtr proc = (*jt);
		MLProc* proc2 = &(*proc);
		MLProcStepSequencer* pSeq = static_cast<MLProcStepSequencer*>(proc2);
		if (pSeq)
		{
			// add bright light for current step of each sequencer
			int step = pSeq->getStep();
			step = clamp(step, 0, 15);
			brightLights |= (1 << step);
			
			// get pattern (same for all)
			if(jt == mSequencerList.begin())
			{
				dimLights = pSeq->getPattern();
			}
		}
	}	
	
	osc::OutboundPacketStream p( mpOSCBuf.data(), kUDPOutputBufferSize );
	
	p << osc::BeginMessage( "/vis/seq" );	
	p << chan << brightLights << dimLights;
	p << osc::EndMessage; 
	
	// debug() << "sending " << p.Size() << " bytes\n";
	if(mSeqInfoSocket.get())
	{
		mSeqInfoSocket->Send( p.Data(), p.Size() );
	}
}


#include "MLProcRMS.h"
const char * kMLRMSProcName("voices_sum_rms");

void MLPluginProcessor::sendVisuals()
{
	float rms = 0.;
	int chan = mT3DHub.getPortOffset();
	//int seqData = mT3DHub.getPortOffset();
	
	MLDSPEngine* eng = getEngine();
	
	eng->getProcList(mRMSProcList, MLPath(kMLRMSProcName), 1);
	
	// debug() << "got " << mRMSProcList.size() << "RMS procs\n";
	// int nRMS = mRMSProcList.size();
	for (MLProcList::const_iterator jt = mRMSProcList.begin(); jt != mRMSProcList.end(); jt++)
	{
		MLProcPtr proc = (*jt);
		MLProc* proc2 = &(*proc);
		MLProcRMS* pRMS = static_cast<MLProcRMS*>(proc2);
		if (pRMS)
		{
			rms = pRMS->getRMS();
			//debug() << "RMS: " << rms << "\n";
		}
	}	
	
	osc::OutboundPacketStream p( mpOSCVisualsBuf.data(), kUDPOutputBufferSize );
	
	p << osc::BeginMessage( "/vis/rms" );	
	p << chan << rms;
	p << osc::EndMessage; 
	
	//debug() << "sending " << p.Size() << " bytes\n";
	if(mVisualsSocket.get())
	{
		mVisualsSocket->Send( p.Data(), p.Size() );
	}
}

// send pgm change message to OSC visuals server
void MLPluginProcessor::sendProgramChange(int pgm)
{
	int chan = mT3DHub.getPortOffset();
	
	osc::OutboundPacketStream p( mpOSCVisualsBuf.data(), kUDPOutputBufferSize );
	
	p << osc::BeginMessage( "/vis/pgm" );	
	p << chan << pgm;
	p << osc::EndMessage; 
	
	//debug() << "sending " << p.Size() << " bytes\n";
	if(mVisualsSocket.get())
	{
		mVisualsSocket->Send( p.Data(), p.Size() );
	}
}

#endif

void MLPluginProcessor::setSequence(const MLSignal& seq)
{
	//debug() << "SEQUENCE: " << seq << "\n";
	for(int i=0; i<16; ++i)
	{
		float step = seq[i];
		MLSymbol stepSym = MLSymbol("seq_pulse").withFinalNumber(i);
		setPropertyImmediate(stepSym, step);
	}
}

