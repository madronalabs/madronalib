
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __PLUGINPROCESSOR__
#define __PLUGINPROCESSOR__

#include "MLDSPEngine.h"
#include "MLAudioProcessorListener.h"
#include "MLDefaultFileLocations.h"
#include "MLModel.h"
#include "MLPluginEditor.h"
#include "MLFileCollection.h"
#include "MLControlEvent.h"
#include "MLProcPatcher.h" // TODO avoid

#include <vector>
#include <map>

const int kMLPatcherMaxTableSize = 64;

class MLPluginProcessor : 
	public AudioProcessor,
    public MLFileCollection::Listener,
	public MLPropertySet
{
public:
    
    enum
    {
        kRequiresSSE2,
        kRequiresSSE3
    };
    
	class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void scaleFilesChanged(const MLFileCollectionPtr) = 0;
		virtual void presetFilesChanged(const MLFileCollectionPtr) = 0;
	};
		
	MLPluginProcessor();
    ~MLPluginProcessor();
	
	// --------------------------------------------------------------------------------
	// plugin description and default preset
	void loadPluginDescription(const char* desc);
	virtual void loadDefaultPreset() = 0;
	
	// initializeProcessor is called after graph is created.
	virtual void initializeProcessor() = 0; 

	// --------------------------------------------------------------------------------
	// preflight and cleanup
	MLProc::err preflight(int requirements = kRequiresSSE2);
	virtual bool wantsMIDI() {return true;}
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
	void setDefaultParameters();
	void reset();

	// --------------------------------------------------------------------------------
    // MLFileCollection::Listener
    virtual void processFile (const MLSymbol collection, const MLFile& f, int idx, int size);

	// --------------------------------------------------------------------------------
    void pushInfoToListeners();
    void setProcessorListener (MLPluginProcessor::Listener* l);

	// --------------------------------------------------------------------------------
	// process
	bool isOKToProcess();
    void convertMIDIToEvents (MidiBuffer& midiMessages, MLControlEventVector & events);
	void setCollectStats(bool k);
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	// --------------------------------------------------------------------------------
	bool hasEditor() const { return true; }
	AudioProcessorEditor* createEditor();
    void editorResized(int w, int h);
    const String getName() const { return MLProjectInfo::projectName; }

	// --------------------------------------------------------------------------------
	// DSP parameters
    int getNumParameters();
	int getParameterIndex (const MLSymbol name);
	float getParameter (int index);
    void setParameter (int index, float newValue);
	void setParameter (MLSymbol paramName, float newValue);
	float getParameterAsLinearProportion (int index);
	void setParameterAsLinearProportion (int index, float newValue);
	void MLSetParameterNotifyingHost (const int parameterIndex, const float newValue);

    const String getParameterName (int index);
	const String symbolToXMLAttr(const MLSymbol sym);
	const MLSymbol XMLAttrToSymbol(const String& str);
	
    const MLSymbol getParameterAlias (int index);
    const MLParamValue getParameterDefault (int index);
    const String getParameterText (int index);
	float getParameterMin (int index);
	float getParameterMax (int index);
	
	MLPublishedParamPtr getParameterPtr (int index);
	MLPublishedParamPtr getParameterPtr (MLSymbol sym);
	const std::string& getParameterGroupName (int index);
    
	// --------------------------------------------------------------------------------
	// signals
	int countSignals(const MLSymbol alias);
	unsigned readSignal(const MLSymbol alias, MLSignal& outSig);

	
	MLProcList& getPatcherList(); // TODO remove

	// --------------------------------------------------------------------------------
	// state
	virtual void getStateAsXML (XmlElement& xml);
	virtual void setStateFromXML(const XmlElement& xmlState, bool setViewAttributes);
	int saveStateAsVersion();
    
	int saveStateOverPrevious();
	void returnToLatestStateLoaded();
	
	String getStateAsText ();
	void setStateFromText (const String& stateStr);

	void getStateInformation (juce::MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    int saveStateToFullPath(const std::string& path);
    void saveStateToRelativePath(const std::string& path);
    
    void loadStateFromPath(const std::string& path);
	void loadStateFromFile(const File& loadFile);

	// --------------------------------------------------------------------------------
	// MIDI programs
	
	void clearMIDIProgramFiles();
	void setMIDIProgramFile(int pgm, File f);
	void setStateFromMIDIProgram (const int pgmIdx);
	void scanMIDIPrograms();
 
	// --------------------------------------------------------------------------------
	// channels
	
    const String getInputChannelName (const int channelIndex) const;
    const String getOutputChannelName (const int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;
	double getTailLengthSeconds() const;
	bool acceptsMidi() const;
    bool producesMidi() const;
	
	// --------------------------------------------------------------------------------
	// factory presets - a VST concept - unimplemented
	// 
    int getNumPrograms()                                        { return 0; }
    int getCurrentProgram()                                     { return 0; }
    void setCurrentProgram (int)                          { }
    const String getProgramName (int)                     { return String::empty; }
    void changeProgramName (int, const String&)   { }

	// --------------------------------------------------------------------------------
	// presets
	
	void scanPresets();
    void prevPreset();
    void nextPreset();
    void advancePreset(int amount);
	
	// --------------------------------------------------------------------------------
    AudioPlayHead::CurrentPositionInfo lastPosInfo;

	// --------------------------------------------------------------------------------

	void setMLListener (MLAudioProcessorListener* const newListener) throw();
    MLProc::err sendMessageToMLListener (unsigned msg, const File& f);

	// --------------------------------------------------------------------------------
	// scales
	
	void loadScale(const File& f);
	void loadDefaultScale();
	virtual void broadcastScale(const MLScale* pScale) = 0;
	
	// --------------------------------------------------------------------------------
	// engine stuff

    MLProcPtr getProcFromEngine();
	MLDSPEngine* getEngine() { return &mEngine; }
	inline void showEngine() { mEngine.dump(); }
	
protected:
	// Engine creates graphs of Processors, does the work
	MLDSPEngine	mEngine;	
	
	typedef std::tr1::shared_ptr<XmlElement> XmlElementPtr;
	XmlElementPtr mpLatestStateLoaded;

	// TODO shared_ptr
	ScopedPointer<XmlDocument> mpPluginDoc;
	String mDocLocationString;
		
private:
	void setCurrentPresetDir(const char* name);
    
	// set the plugin state from a memory blob containing parameter and patcher settings.
	void setStateFromBlob (const void* data, int sizeInBytes);
	
	MLAudioProcessorListener* MLListener;
    Listener* mpListener;
    
	// number of parameters stored here so we can access it before engine compile
	int mNumParameters; 
	
	// True when any parameters have been set by the host. 
	// If the host doesn't give us a program to load, we can use this to 
	// decide to load defaults after compile.
	bool mHasParametersSet;

	// temp storage for parameter data given to us before our DSP graph is made.
	juce::MemoryBlock mSavedParamBlob;	
	
	String mCurrentPresetName;
	String mCurrentPresetDir;

    MLFileCollectionPtr mScaleFiles;
    MLFileCollectionPtr mPresetFiles;
    
	File mFactoryPresetsFolder, mUserPresetsFolder;
	bool mFileLocationsOK;
		
	std::vector<File> mMIDIProgramFiles;
	
	int mWrapperFormat;
	
	// saved state for editor
	MLRect mEditorRect;
	bool mEditorNumbersOn;
	bool mEditorAnimationsOn;
	
	bool mInitialized;
    
    // vector of control events to send to engine along with each block of audio.
    MLControlEventVector mControlEvents;
};

#endif  // __PLUGINPROCESSOR__
