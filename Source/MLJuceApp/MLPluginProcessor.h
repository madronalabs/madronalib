
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __PLUGINPROCESSOR__
#define __PLUGINPROCESSOR__

#include "MLDSPEngine.h"
#include "MLAudioProcessorListener.h"
#include "MLDefaultFileLocations.h"
#include "MLModel.h"
#include "MLFileCollection.h"
#include "MLControlEvent.h"
#include "MLProperty.h" 
#include "MLAppState.h"
#include "MLStringUtils.h"

#if ML_MAC
#include "pa_ringbuffer.h"
#include "UdpSocket.h"
#include "OscReceivedElements.h"
#include "OscPacketListener.h"
#include "MLNetServiceHub.h"
#include "MLT3DHub.h"
#endif

#include <vector>
#include <map>

const int kMLPatcherMaxTableSize = 64;
const int kMLPluginMIDIPrograms = 127;

class MLPluginProcessor : 
	public AudioProcessor,
#if ML_MAC
	public MLT3DHub::Listener,
	public MLNetServiceHub,
#endif
	public MLModel
{
	friend class MLEnvironmentModel;
public:
    enum
    {
        kRequiresSSE2,
        kRequiresSSE3
    };
	
	class MLEnvironmentModel : public MLModel
	{
	public:
		MLEnvironmentModel(MLPluginProcessor* pProc) : mpOwnerProcessor(pProc) {}
		~MLEnvironmentModel() {}
		void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
		
		MLPluginProcessor* mpOwnerProcessor;
	};
    
	MLPluginProcessor();
    ~MLPluginProcessor();

	// MLModel implementation
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	// juce::AudioProcessor implementation
	const String getName() const { return MLProjectInfo::projectName; }
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    const String getInputChannelName (const int channelIndex) const;
    const String getOutputChannelName (const int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;
	double getTailLengthSeconds() const;
	bool acceptsMidi() const;
    bool producesMidi() const;
	void reset();
	AudioProcessorEditor* createEditor();
	bool hasEditor() const { return true; }
	int getNumParameters();
    const String getParameterName (int index);
	float getParameter (int index);
    const String getParameterText (int index);
	float getParameterDefaultValue (int index);
    void setParameter (int index, float newValue);
	bool isParameterAutomatable (int idx) const;
	
	// factory presets - a VST concept - TODO
    int getNumPrograms() { return 0; }
    int getCurrentProgram() { return 0; }
    void setCurrentProgram (int) { }
    const String getProgramName (int) { return String::empty; }
    void changeProgramName (int, const String&) { }
	
	void getStateInformation (juce::MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);
	
    void editorResized(int w, int h);
    
	// plugin description and default preset
	void loadPluginDescription(const char* desc);
	virtual void loadDefaultPreset() = 0;
	
	// initializeProcessor is called after graph is created.
	virtual void initializeProcessor(); 

	// preflight and cleanup
	MLProc::err preflight(int requirements = kRequiresSSE2);
	virtual bool wantsMIDI() {return true;}
	void setDefaultParameters();
	
	// add an additional listener to the file collections that we have. Controllers can use this
	// to get updates and build menus, etc.
	void addFileCollectionListener(MLFileCollection::Listener* pL);
	
	// MLT3DHub::Listener
#if ML_MAC
	void handleHubNotification(MLSymbol action, const float val);
#endif
	// process
	bool isOKToProcess();
    void convertMIDIToEvents (MidiBuffer& midiMessages, MLControlEventVector & events);
	void setCollectStats(bool k);

	// parameters
	int getParameterIndex (const MLSymbol name);
	float getParameterAsLinearProportion (int index);
	void setParameterAsLinearProportion (int index, float newValue);
    const MLSymbol getParameterAlias (int index);
    float getParameterMin (int index);
	float getParameterMax (int index);
	MLPublishedParamPtr getParameterPtr (int index);
	MLPublishedParamPtr getParameterPtr (MLSymbol sym);
	const std::string& getParameterGroupName (int index);

	// signals
	int countSignals(const MLSymbol alias);

	// get the patch and environment states as a binary blob.
	void getPatchAndEnvStatesAsBinary (juce::MemoryBlock& destData);
	// set the patch and environment states from a binary blob.
	void setPatchAndEnvStatesFromBinary (const void* data, int sizeInBytes);
    
	int saveStateAsVersion();
	int saveStateOverPrevious();
	void returnToLatestStateLoaded();

	String getStateAsText();
	void setPatchStateFromText(const String& stateStr);
    void saveStateToFullPath(const std::string& path);
    void saveStateToRelativePath(const std::string& path);
    
	void loadStateFromPath(const std::string& path);
	void loadPatchStateFromMIDIProgram (const int pgmIdx);
	void loadPatchStateFromFile(const MLFile& loadFile);
	
	// deprecated - to remove in 2.0
	virtual void setStateFromXML(const XmlElement& xmlState, bool setViewAttributes);
	const String symbolToXMLAttr(const MLSymbol sym);
	const MLSymbol XMLAttrToSymbol(const String& str);
 
	// files
	void createFileCollections();
	void scanAllFilesImmediate();
	const MLFileCollectionPtr& getPresetCollection() const { return mPresetFiles; }
		
	// presets
    void prevPreset();
    void nextPreset();
    void advancePreset(int amount);
	
	void setMLListener (MLAudioProcessorListener* const newListener) throw();
    MLProc::err sendMessageToMLListener (unsigned msg, const File& f);
	
	// engine stuff
	MLDSPEngine* getEngine() { return &mEngine; }
	inline void showEngine() { mEngine.dump(); }
	
	// environment: through which anything outside the patch, such as window size, can be stored in the host
	MLEnvironmentModel* getEnvironment() { return mpEnvironmentModel.get(); }
	
protected:
	// set what kind of event input we are listening to (MIDI or OSC)
	void setInputProtocol(int p);

	// set the parameter of the Engine but not the Model property.
	void setParameterWithoutProperty (MLSymbol paramName, float newValue);
	void setStringParameterWithoutProperty (MLSymbol paramName, const std::string& newValue);
	void setSignalParameterWithoutProperty (MLSymbol paramName, const MLSignal& newValue);
		
	// Engine creates graphs of Processors, does the work
	MLDSPEngine	mEngine;	

	int mInputProtocol;
		
	typedef std::tr1::shared_ptr<XmlElement> XmlElementPtr;
	ScopedPointer<XmlDocument> mpPluginDoc;
	String mDocLocationString;
    
	AudioPlayHead::CurrentPositionInfo lastPosInfo;
		
private:
	
	void setCurrentPresetDir(const char* name);
    
	MLAudioProcessorListener* mMLListener;
    
	// The number of parameters in the plugin is stored here so we can access it before
	// the DSP engine is compiled.
	int mNumParameters; 
	
	// True when any parameters have been set by the host. 
	// If the host doesn't give us a program to load, we can use this to 
	// decide to load defaults after compile.
	bool mHasParametersSet;

	// temp storage for parameter data given to us before our DSP graph is made.
	juce::MemoryBlock mSavedBinaryState;	
	
	String mCurrentPresetName;
	String mCurrentPresetDir;

    MLFileCollectionPtr mScaleFiles;
    MLFileCollectionPtr mPresetFiles;
    MLFileCollectionPtr mMIDIProgramFiles;
	
	// saved state for editor
	MLRect mEditorRect;
	bool mEditorNumbersOn;
	bool mEditorAnimationsOn;
	
	bool mInitialized;
    
    // vector of control events to send to engine along with each block of audio.
    MLControlEventVector mControlEvents;
	
	MLAppStatePtr mpPatchState;
	std::tr1::shared_ptr<MLEnvironmentModel> mpEnvironmentModel;
	MLAppStatePtr mpEnvironmentState;
#if defined(__APPLE__)
	MLT3DHub mT3DHub;
#endif

};

#endif  // __PLUGINPROCESSOR__
