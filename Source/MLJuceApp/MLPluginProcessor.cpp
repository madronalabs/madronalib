
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginProcessor.h"
const int kMaxControlEventsPerBlock = 1024;

MLPluginProcessor::MLPluginProcessor() : 
	MLListener(0),
    mpListener(0),
	mEditorNumbersOn(true),
	mEditorAnimationsOn(true),
	mInitialized(false),
	mInputProtocol(-1),
	mT3DWaitTime(0),
	mDataRate(-1)
{
	mHasParametersSet = false;
	mNumParameters = 0;
	lastPosInfo.resetToDefault();
    
    // get scales collection
    mScaleFiles = MLFileCollectionPtr(new MLFileCollection("scales", getDefaultFileLocation(kScaleFiles), "scl"));
    mScaleFiles->setListener(this);
    mScaleFiles->searchForFilesNow();
    
    scanPresets();
	scanMIDIPrograms();
    
    mControlEvents.resize(kMaxControlEventsPerBlock);
}

MLPluginProcessor::~MLPluginProcessor()
{
//	debug() << "deleting MLPluginProcessor.\n";
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
			//debug() << "loaded " << JucePlugin_Name << " plugin description, " << mNumParameters << " parameters.\n";	
		}
		else
		{
			MLError() << "MLPluginProcessor: error loading plugin description!\n";
		}   
	}
	else
	{
		MLError() << "MLPluginProcessor: couldn't load plugin description!\n";
	}
}

// --------------------------------------------------------------------------------
// editor creation function to be defined in (YourPluginEditor).cpp
//
extern MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const ownerProcessor, const MLRect& bounds, bool num, bool anim);

AudioProcessorEditor* MLPluginProcessor::createEditor()
{
	// creation function defined to return the plugin's flavor of plugin editor
    MLPluginEditor* r = CreateMLPluginEditor(this, mEditorRect, mEditorNumbersOn, mEditorAnimationsOn);	
	return r;
}

void MLPluginProcessor::editorResized(int w, int h)
{
    mEditorRect.setWidth(w);
    mEditorRect.setHeight(h);
}

// --------------------------------------------------------------------------------
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
		const ScopedLock sl (getCallbackLock());

		unsigned inChans = getNumInputChannels();
		unsigned outChans = getNumOutputChannels();
		mEngine.setInputChannels(inChans);
		mEngine.setOutputChannels(outChans);

		unsigned bufSize = 0;
		unsigned chunkSize = 0;

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
		
		// debug() << "MLPluginProcessor: prepareToPlay: rate " << sr << ", buffer size " << bufSize << ", vector size " << vecSize << ". \n";	
		
		// build: turn XML description into graph of processors
		if (mEngine.getGraphStatus() != MLProc::OK)
		{
			bool makeSignalInputs = inChans > 0;
			r = mEngine.buildGraphAndInputs(&*mpPluginDoc, makeSignalInputs, wantsMIDI()); 
			// debug() << getNumParameters() << " parameters in description.\n";
		}
		else
		{
			// debug() << "MLPluginProcessor graph OK.\n";
		}

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
		
		// mEngine.dump();
			
		// after prepare to play, set state from saved blob if one exists
		const unsigned blobSize = mSavedParamBlob.getSize();
		if (blobSize > 0)
		{
			setStateFromBlob (mSavedParamBlob.getData(), blobSize);
			mSavedParamBlob.setSize(0);
		}
		else 
		{
			mEngine.clear();
			if (!mHasParametersSet)
			{
				loadDefaultPreset();
			}
		}		
		
		if(!mInitialized)
		{					
			initializeProcessor();
			mInitialized = true;
		}		
		
		mEngine.setEnabled(prepareErr == MLProc::OK);
	}
}

void MLPluginProcessor::reset()
{
	const ScopedLock sl (getCallbackLock());
	mEngine.clear();
}

void MLPluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

// --------------------------------------------------------------------------------
#pragma mark MLFileCollection::Listener

void MLPluginProcessor::processFile (const MLSymbol collection, const MLFile& f, int idx, int size)
{
    if(collection == "scales")
    {
    }
    else if(collection == "mappings")
    {
    }
    else if(collection == "presets")
    {
        if(idx == size) // done?
        {
            pushInfoToListeners();
        }
    }
}

// --------------------------------------------------------------------------------
#pragma mark Listener related

void MLPluginProcessor::pushInfoToListeners()
{
    // push things a listener needs to know about
    if(mpListener)
    {
        // sample file collection
        mpListener->scaleFilesChanged(mScaleFiles);
        mpListener->presetFilesChanged(mPresetFiles);
    }
}

// when a new listener is set, immediately update it with all the current info we have.
void MLPluginProcessor::setProcessorListener(MLPluginProcessor::Listener* l)
{
    mpListener = l;
    pushInfoToListeners();
}

// --------------------------------------------------------------------------------
#pragma mark process

void MLPluginProcessor::convertMIDIToEvents (MidiBuffer& midiMessages, MLControlEventVector& events)
{
    int c = 0;
    int size = events.size();
    
	MidiBuffer::Iterator i (midiMessages);
    juce::MidiMessage message (0xf4, 0.0);
    MLControlEvent::EventType type = MLControlEvent::eNull;
    int chan = 0;
    int id = 0;
    int time = 0;
    float v1 = 0.f;
    float v2 = 0.f;
		
    while (i.getNextEvent(message, time)) // writes to time
	{
        chan = message.getChannel();
		if (message.isNoteOn())
		{
            type = MLControlEvent::eNoteOn;
			v1 = message.getNoteNumber();
			v2 = message.getVelocity() / 127.f;
            id = (int)v1;
		}
		else if(message.isNoteOff())
		{
            type = MLControlEvent::eNoteOff;
			v1 = message.getNoteNumber();
			v2 = message.getVelocity() / 127.f;
            id = (int)v1;
		}
		else if (message.isController())
		{
            type = MLControlEvent::eController;
			v1 = message.getControllerNumber();
			v2 = message.getControllerValue() / 127.f;
		}
		else if (message.isPitchWheel())
		{
            type = MLControlEvent::ePitchWheel;
			v1 = message.getPitchWheelValue();
		}
		else if (message.isAftertouch())
		{
            type = MLControlEvent::eNotePressure;
			v1 = message.getNoteNumber();
			v2 = message.getAfterTouchValue() / 127.f;
            id = (int)v1;
		}
		else if (message.isChannelPressure())
		{
            type = MLControlEvent::eChannelPressure;
			v1 = message.getChannelPressureValue() / 127.f;
		}
		else if (message.isSustainPedalOn())
		{			
            type = MLControlEvent::eSustainPedal;
			v1 = 1.f;
		}
		else if (message.isSustainPedalOff())
		{
            type = MLControlEvent::eSustainPedal;
			v1 = 0.f;
		}
		else if (message.isProgramChange())
		{
			int pgm = message.getProgramChangeNumber();
//debug() << "program change " << pgm << "\n";
			if(pgm == kMLPluginMIDIPrograms)	
			{
				// load most recent saved program
				returnToLatestStateLoaded();
			}
			else
			{		
				pgm = clamp(pgm, 0, kMLPluginMIDIPrograms - 1);			
				setStateFromMIDIProgram(pgm);
			}
            type = MLControlEvent::eProgramChange;
            id = chan;
            v1 = (float)pgm;
		}
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
        if(c < size - 1)
        {
            events[c++] = MLControlEvent(type, chan, id, time, v1, v2);
        }
	}
    
    // null-terminate new event list
    events[c] = kMLNullControlEvent;
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
		
		// TEST
		if(0)
		if(lastPosInfo.isPlaying)
		{
			debug() << "bpm:" << lastPosInfo.bpm 
			<< " ppq:" << std::setprecision(5) << ppqPosition << std::setprecision(2) 
			<< " secs:" << secsPosition << "\n";
		}
			
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
            convertMIDIToEvents(midiMessages, mControlEvents);
            midiMessages.clear(); // otherwise messages will be passed back to the host
        }
        mEngine.processBlock(samples, mControlEvents, samplesPosition, secsPosition, ppqPosition, bpm, isPlaying);
    }
	else
	{
		buffer.clear();
	}
}

// --------------------------------------------------------------------------------
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
  	if (index < 0) return(0);
	return mEngine.getParamByIndex(index);
}

// set plugin parameter by index. Typically called by the host wrapper
//
void MLPluginProcessor::setParameter (int index, float newValue)
{
	if (index < 0) return;	

	mEngine.setPublishedParam(index, newValue);	
	mHasParametersSet = true;
	setPropertyImmediate(getParameterAlias(index), newValue);
}

// set plugin parameter by name without setting property. Typically called from internal code.
//
void MLPluginProcessor::setParameterWithoutProperty (MLSymbol paramName, float newValue)
{
	int index = getParameterIndex(paramName);
	if (index < 0) return;	

	mEngine.setPublishedParam(index, newValue);	
	mHasParametersSet = true;
}

// for VST wrapper.
float MLPluginProcessor::getParameterAsLinearProportion (int index)
{
	float r = 0;
  	if (index < 0) return(0);
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
	if (index < 0) return;	
	MLPublishedParamPtr p = mEngine.getParamPtr(index);
	if(p)
	{
		p->setValueAsLinearProportion(newValue);	
		mEngine.setPublishedParam(index, p->getValue());	
		mHasParametersSet = true;
		
		// set MLModel Parameter 
		MLSymbol paramName = getParameterAlias(index);
		float realVal = mEngine.getParamByIndex(index);
		setPropertyImmediate(paramName, realVal);
	}
}


float MLPluginProcessor::getParameterMin (int index)
{
	if (index < 0) return(0);
	return mEngine.getParamPtr(index)->getRangeLo();
}

float MLPluginProcessor::getParameterMax (int index)
{
	if (index < 0) return(0);
	return mEngine.getParamPtr(index)->getRangeHi();
}

const String MLPluginProcessor::getParameterName (int index)
{
	MLSymbol nameSym;
	const int p = mEngine.getPublishedParams(); 
	if (index < mNumParameters)
	{
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
 	return mEngine.getParamPtr(index)->getAlias();
}

const MLParamValue MLPluginProcessor::getParameterDefault (int index)
{
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

// --------------------------------------------------------------------------------
#pragma mark MLModel

void MLPluginProcessor::doPropertyChangeAction(MLSymbol property, const MLProperty& newVal)
{
	int propertyType = newVal.getType();
	int paramIdx = getParameterIndex(property);
	float f = newVal.getFloatValue();
	
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
			setParameterWithoutProperty (property, f);
			if (paramIdx < 0) return;
			
			// convert to host units for VST
			f = newVal.getFloatValue();
			if (wrapperType == AudioProcessor::wrapperType_VST)
			{
				MLPublishedParamPtr p = mEngine.getParamPtr(paramIdx);
				if(p)
				{
					f = p->getValueAsLinearProportion();
				}
			}
			// send to wrapper in host units
			AudioProcessor::sendParamChangeMessageToListeners (paramIdx, f);
			
			break;
		case MLProperty::kStringProperty:
			break;
		case MLProperty::kSignalProperty:
			break;
		default:
			break;
	}
}

// --------------------------------------------------------------------------------
#pragma mark signals
//

// count the number of published copies of the signal matching alias.
int MLPluginProcessor::countSignals(const MLSymbol alias)
{
	int numSignals = mEngine.getPublishedSignalVoicesEnabled(alias);
	return numSignals;
}

// fill the output signal with samples from the named published signal list.
// returns the number of samples read.
unsigned MLPluginProcessor::readSignal(const MLSymbol alias, MLSignal& outSig)
{
	unsigned samples = mEngine.readPublishedSignal(alias, outSig);
	return samples;
}

// --------------------------------------------------------------------------------
#pragma mark patcher-specific TO REMOVE
//

MLProcList& MLPluginProcessor::getPatcherList()
{
 	return mEngine.getPatcherList();
}

// --------------------------------------------------------------------------------
#pragma mark state

void MLPluginProcessor::getStateAsXML (XmlElement& xml)
{
	if( !(mEngine.getCompileStatus() == MLProc::OK)) return;
	
#if DEMO	
	xml.setAttribute ("pluginVersion", JucePlugin_VersionCode);	
    xml.setAttribute ("presetName", String("----"));	
#else

  	const unsigned numParams = getNumParameters();

	// TODO use string properties of model instead of these JUCE strings.
	// also move to JSON.
	xml.setAttribute ("pluginVersion", JucePlugin_VersionCode);
	xml.setAttribute ("presetName", String(getStringProperty("preset").c_str()));
	xml.setAttribute ("scaleName", String(getStringProperty("key_scale").c_str()));

	// store parameter values to xml as a bunch of attributes.
	// not XML best practice in general but takes fewer characters.
	for(unsigned i=0; i<numParams; ++i)
	{
		const String paramName = symbolToXMLAttr(getParameterAlias(i));
		const float defaultVal = getParameterDefault(i);
		const float paramVal = getParameter(i);
		if (paramVal != defaultVal)
		{
			xml.setAttribute(paramName, paramVal);		
			//debug() << "setting XML param " << paramName << " to " << paramVal << "\n";
		}
	}

	// store patcher info to xml
	{			
		MLProcList patchers = getPatcherList();
		if (!patchers.empty())
		{
			MLProcPatcher& firstPatcher = static_cast<MLProcPatcher&>(**patchers.begin());
			const int inputs = firstPatcher.getParam("inputs");
			const int outputs = firstPatcher.getParam("outputs");
			String outStr;
			String patcherInput = "patcher_input_";
			
			for(unsigned i=1; i<=inputs; ++i)
			{
				bool differentFromDefault = false;
				outStr = "";
				for(unsigned j=1; j<=outputs; ++j)
				{
					if (firstPatcher.getConnection(i, j))
					{
						outStr += "1";
						differentFromDefault = true;
					}
					else
					{
						outStr += "0";
					}
				}
				if(differentFromDefault)
				{
					String outNum (i); 
					xml.setAttribute(patcherInput + outNum, outStr);	
				}				
			}
		}
	}	
	
	// store editor state to XML if one exists	
	MLPluginEditor* pEditor = static_cast<MLPluginEditor*>(getActiveEditor());
	if(pEditor)
	{
		MLRect r = pEditor->getWindowBounds();
		xml.setAttribute("editor_x", r.x());	
		xml.setAttribute("editor_y", r.y());	
		xml.setAttribute("editor_width", r.getWidth());	
		xml.setAttribute("editor_height", r.getHeight());
		xml.setAttribute("editor_num", getFloatProperty("patch_num"));	
		xml.setAttribute("editor_anim", getFloatProperty("patch_anim"));	
	}
	
	// save blob as most recently saved state
	mpLatestStateLoaded = XmlElementPtr(new XmlElement(xml));
	
#endif

}

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

void MLPluginProcessor::returnToLatestStateLoaded()
{
	if(mpLatestStateLoaded)
	{
        bool setViewAttributes = false;
		setStateFromXML(*mpLatestStateLoaded, setViewAttributes);
	}
	else
	{
		debug() << "MLPluginProcessor::returnToLatestStateLoaded: no saved state!\n";
	}
}

void MLPluginProcessor::setStateFromXML(const XmlElement& xmlState, bool setViewAttributes)
{
	if (!(xmlState.hasTagName (JucePlugin_Name))) return;
	if (!(mEngine.getCompileStatus() == MLProc::OK)) return; // TODO revisit need to compile first

	// getCallbackLock() is in juce_AudioProcessor
	// process lock is a quick fix.  it is here to prevent doParams() from getting called in 
	// process() methods and thereby setting mParamsChanged to false before the real changes take place.
	// A better alternative would be a lock-free queue of parameter changes.
	const ScopedLock sl (getCallbackLock()); 
		
	// only the differences between default parameters and the program state are saved in a program,
	// so the first step is to set the default parameters.
	setDefaultParameters();
	
	// get program version of saved state
	unsigned blobVersion = xmlState.getIntAttribute ("pluginVersion");
	unsigned pluginVersion = JucePlugin_VersionCode;
	
	if (blobVersion > pluginVersion)
	{
		// TODO show error to user
		MLError() << "MLPluginProcessor::setStateFromXML: saved program version is newer than plugin version!\n";
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
    bool loaded = false;
    // look for scale under full name with path
    if(fullScaleName != std::string())
    {
        const MLFilePtr f = mScaleFiles->getFileByName(fullScaleName);
        if(f != MLFilePtr())
        {
            loadScale(f->mFile);
            loaded = true;
        }
    }
    if(!loaded)
    {
        loadDefaultScale();
    }
    
	// get preset name saved in blob.  when saving from AU host, name will also be set from RestoreState().
	const String presetName = xmlState.getStringAttribute ("presetName");
	setProperty("preset", std::string(presetName.toUTF8()));
    
	/*
     debug() << "MLPluginProcessor: setStateFromXML: loading program " << presetName << ", version " << std::hex << blobVersion << std::dec << "\n";
     MemoryOutputStream myStream;
     xmlState->writeToStream (myStream, "");
     debug() << myStream.toString();
     */
	
    /*
     setCurrentPresetName(presetName.toUTF8());
     setCurrentPresetDir(presetDir.toUTF8());
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
	
	// get params from xml
	const unsigned numAttrs = xmlState.getNumAttributes();
	String patcherInputStr ("patcher_input_");

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
				setPropertyImmediate(paramSym, paramVal);
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
						setPropertyImmediate(newSym, paramVal);
					}
					else
					{
						MLError() << "MLPluginProcessor::setStateFromXML: no such parameter! \n";
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

String MLPluginProcessor::getStateAsText ()
{
    // create an outer XML element.
	ScopedPointer<XmlElement> xmlProgram (new XmlElement(JucePlugin_Name));
	getStateAsXML(*xmlProgram);
	
	return xmlProgram->createDocument (String::empty, true, false);
}

void MLPluginProcessor::setStateFromText (const String& stateStr)
{
	XmlDocument doc(stateStr);
	XmlElementPtr xmlState (doc.getDocumentElement());
	if (xmlState)
	{
        bool setViewAttributes = false;
		setStateFromXML(*xmlState, setViewAttributes);
		mpLatestStateLoaded = xmlState;
	}
}

void MLPluginProcessor::getStateInformation (MemoryBlock& destData)
{
    // create an outer XML element.
    ScopedPointer<XmlElement> xmlProgram (new XmlElement(JucePlugin_Name));
	getStateAsXML(*xmlProgram);
    copyXmlToBinary (*xmlProgram, destData);
}

void MLPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// if uninitialized, save blob for later.
	if (mEngine.getCompileStatus() != MLProc::OK)
	{
		mSavedParamBlob.setSize(0);
		mSavedParamBlob.append(data, sizeInBytes);
	}
	else
	{
		setStateFromBlob (data, sizeInBytes);
	}
}

// save with full path name as input -- needed for using system File dialogs
//
int MLPluginProcessor::saveStateToFullPath(const std::string& fullPath)
{
    int r = 0;
    std::string newPath = mPresetFiles->getRelativePath(fullPath);
    if(newPath != "")
    {
        saveStateToRelativePath(newPath);
    }
    else
    {
        r = 1;
    }
    return r;
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
    std::string shortPath = stripExtension(path);
    setProperty("preset", shortPath);

    std::string extension (".mlpreset");
    std::string extPath = shortPath + extension;
    const MLFilePtr f = mPresetFiles->createFile(extPath);
    if(!f->mFile.exists())
    {
        f->mFile.create();
    }

    f->mFile.replaceWithText(getStateAsText());
#endif // DEMO
    
}

void MLPluginProcessor::loadStateFromPath(const std::string& path)
{    
    if(path != std::string())
    {
        const MLFilePtr f = mPresetFiles->getFileByName(path);
        if(f != MLFilePtr())
        {
            loadStateFromFile(f->mFile);
            std::string shortPath = stripExtension(path);
            setProperty("preset", shortPath);
        }
    }
}

void MLPluginProcessor::loadStateFromFile(const File& f)
{
	if (f.exists())
	{
		String extension = f.getFileExtension();
		if (extension == ".mlpreset")
		{
			// load cross-platform mlpreset file.
			ScopedPointer<XmlDocument> stateToLoad (new XmlDocument(f));
			if (stateToLoad != NULL)
			{
				XmlElementPtr pDocElem (stateToLoad->getDocumentElement(true));
				setStateFromXML(*pDocElem, false);
				mpLatestStateLoaded = pDocElem;
			}
		}        
		else if (extension == ".aupreset")
		{
			// tell AU wrapper to load AU-compatible .aupreset file.
			sendMessageToMLListener (MLAudioProcessorListener::kLoad, f);
		}
	}	
}

// set state, including the view status, from a binary blob stored in host.
//
void MLPluginProcessor::setStateFromBlob (const void* data, int sizeInBytes)
{
	debug() << "setStateFromBlob: " << sizeInBytes << "bytes of XML data.\n";
	XmlElementPtr xmlState(getXmlFromBinary (data, sizeInBytes));
	if (xmlState)
	{
        bool setViewAttributes = true;
		setStateFromXML(*xmlState, setViewAttributes);
		mpLatestStateLoaded = xmlState;
	}
}

// --------------------------------------------------------------------------------
#pragma mark MIDI programs

void MLPluginProcessor::clearMIDIProgramFiles()
{
	mMIDIProgramFiles.clear();
	mMIDIProgramFiles.resize(kMLPluginMIDIPrograms);
}

void MLPluginProcessor::setMIDIProgramFile(int idx, File f)
{
	if(idx < kMLPluginMIDIPrograms)
	{
		mMIDIProgramFiles[idx] = f;
	}
}

void MLPluginProcessor::setStateFromMIDIProgram (const int idx)
{
	if(within(idx, 0, kMLPluginMIDIPrograms))
	{
		if(mMIDIProgramFiles[idx].exists())
		{
			loadStateFromFile(mMIDIProgramFiles[idx]);
		}
	}
}

void MLPluginProcessor::scanPresets()
{
    // get presets collections
    mPresetFiles = MLFileCollectionPtr(new MLFileCollection("presets", getDefaultFileLocation(kPresetFiles), "mlpreset"));
    mPresetFiles->setListener(this);
    mPresetFiles->searchForFilesNow();
}

void MLPluginProcessor::scanMIDIPrograms()
{
	String presetFileType = ".mlpreset";
    
	clearMIDIProgramFiles();
	
	File startDir = getDefaultFileLocation(kPresetFiles);
	if (!startDir.isDirectory()) return;	
	File subDir = startDir.getChildFile("MIDI Programs");
	if (!subDir.isDirectory()) 
	{
		// debug() << "WARNING: MIDI Programs directory not found.\n";
		return;	
	}
	Array<File> subdirArray;
	const int level1FilesToFind = File::findFiles;
	int filesInCategory = subDir.findChildFiles(subdirArray, level1FilesToFind, false);
	int midiPgmCount = 0;
	for(int j=0; j<filesInCategory; ++j)
	{
		File f2 = subdirArray[j];
		if (f2.hasFileExtension(presetFileType))
		{
			if(midiPgmCount < kMLPluginMIDIPrograms)
			{
				setMIDIProgramFile(midiPgmCount++, f2);
			}
		}
	}
	// debug() << "MLPluginProcessor::scanMIDIPrograms found " << midiPgmCount << " MIDI programs\n";
}

// --------------------------------------------------------------------------------
#pragma mark presets
//

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
    int len = mPresetFiles->size();
    std::string extension (".mlpreset");

    int currIdx = - 1;
	currIdx = mPresetFiles->getFileIndexByName(getStringProperty("preset") + extension);
    
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
    std::string relPath = mPresetFiles->getFileNameByIndex(currIdx);
    loadStateFromPath(relPath);
}

// set default value for each scalar parameter.  needed before loading
// patches, which are only required to store differences from these
// default values.
void MLPluginProcessor::setDefaultParameters()
{
	if (mEngine.getCompileStatus() == MLProc::OK)
	{	
		// set default for each parameter.
		const unsigned numParams = getNumParameters();
		for(unsigned i=0; i<numParams; ++i)
		{
			float defaultVal = getParameterDefault(i);
			setPropertyImmediate(getParameterAlias(i), defaultVal);
		}
	}
}

// --------------------------------------------------------------------------------
#pragma mark channels
//

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
	return 1.f;
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
    MLListener = newListener;
}

MLProc::err MLPluginProcessor::sendMessageToMLListener (unsigned msg, const File& f)
{
	MLProc::err err = MLProc::OK;
	if(!MLListener) return MLProc::unknownErr;

	switch(msg)
	{
		case MLAudioProcessorListener::kLoad:
			MLListener->loadFile (f);
		break;
		case MLAudioProcessorListener::kSave:
			MLListener->saveToFile (f);
		break;
		default:
		break;
	}
	return err;
}

// TODO what is this doing in Processor? Should be in MLScale.
void MLPluginProcessor::loadScale(const File& f) 
{
	MLScale* pScale = mEngine.getScale();
	if (!pScale) return;

	String scaleName = f.getFileNameWithoutExtension();
	String scaleDir = f.getParentDirectory().getFileNameWithoutExtension();
	String scaleStr = f.loadFileAsString();
	const String delim = "\n\r";
	int a, b;
	int contentLines = 0;
	int ratios = 0;

	// debug() << "MLPluginProcessor: loading scale " << scaleDir << "/" << scaleName << "\n";		

	a = b = 0;
	while(b >= 0)
	{
		b = scaleStr.indexOfAnyOf(delim, a, false);
		
//		debug() << "[" << a << ", " << b << "] > " << scaleStr.substring(a, b) << "\n";
		String inputLine = scaleStr.substring(a, b);
		
		if (inputLine[0] != '!')
		{
			contentLines++;
			
			switch(contentLines)
			{
				case 1:
				{
					const char* descStr = inputLine.toUTF8();
					pScale->setDescription(descStr);
					const char* nameStr = scaleName.toUTF8();
					pScale->setName(nameStr);
				}
				break;
				
				case 2:
				{
//					int notes = inputLine.getIntValue(); // unused
					pScale->clear();
				}
				break;
				
				default: // after 2nd line, add ratios.
				{
					// 
					if (inputLine.contains("."))
					{
						double ratio = inputLine.getDoubleValue();
						ratios++;
						pScale->addRatio(ratio);
					}
					else
					{
						if (inputLine.containsChar('/'))
						{
							int s = inputLine.indexOfChar('/');
							String numStr = inputLine.substring(0, s);
							String denomStr = inputLine.substring(s + 1, inputLine.length());
							int num = numStr.getIntValue();
							int denom = denomStr.getIntValue();
//	debug() << "n:" << num << " denom: " << denom << "\n";
							if ((num > 0) && (denom > 0))
							{
								ratios++;
								pScale->addRatio(num, denom);
							}
						}
						else
						{
							int num = inputLine.getIntValue();
							if (num > 0)
							{
								ratios++;
								pScale->addRatio(num, 1);
							}
						}
					}
				}
				break;			
			}
		}
		
		a = b + 2;	
	}
	
	if (ratios > 0)
	{
		// TODO load .kbm mapping file if one exists
		pScale->setDefaultMapping();
		pScale->recalcRatios();		
	}
	
	broadcastScale(pScale);
}

void MLPluginProcessor::loadDefaultScale()
{
	MLScale* pScale = mEngine.getScale();
	if (!pScale) return;
	
	pScale->setDefaultScale();
	pScale->setDefaultMapping();
	pScale->recalcRatios();		
	broadcastScale(pScale);
} 


// called to change the input protocol, or ping that t3d is alive.
//
void MLPluginProcessor::setInputProtocol(int p)
{
	if(p != mInputProtocol)
	{
		// set the model’s protocol property, which a View can use to change its UI
		setProperty("protocol", p);
		getEngine()->setEngineInputProtocol(p);
		switch(p)
		{
			case kInputProtocolMIDI:
				break;
			case kInputProtocolOSC:
				break;
		}
		mInputProtocol = p;
	}
}


/*
// if we are in t3d mode and get no pings for a while, switch back
// to MIDI mode assuming Soundplane or t3d device was disconnected.
//
void AaltoProcessor::ProtocolPoller::timerCallback()


yuck
 
 restore this, and finally Patcher.

{
	 static const int kT3DTimeout = 4;
#if ML_MAC
	 PollNetServices();
#endif
	 if(mInputProtocol == kInputProtocolOSC)
	 {
		 // increment counter. this is reset each time we receive a t3d frame.
		 mT3DWaitTime++;
		 // debug() << "waiting for t3d:\n";
		 if(mT3DWaitTime > kT3DTimeout)
		 {
			 debug() << "t3d timeout, switching back to MIDI.\n";
			 setInputProtocol(kInputProtocolMIDI);
		 }
	 }
 }

*/



