
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

#include "OscOutboundPacketStream.h"
#include "UdpSocket.h"

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
	virtual void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	// juce::AudioProcessor
	const String getName() const override { return MLProjectInfo::projectName; }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;

    const String getInputChannelName (const int channelIndex) const override;
    const String getOutputChannelName (const int channelIndex) const override;
    bool isInputChannelStereoPair (int index) const override;
    bool isOutputChannelStereoPair (int index) const override;

	bool silenceInProducesSilenceOut() const override { return false; }
	double getTailLengthSeconds() const override;

	bool acceptsMidi() const override;
    bool producesMidi() const override;
	
	void reset() override;

	AudioProcessorEditor* createEditor() override;

	bool hasEditor() const override { return true; }
	int getNumParameters() override;
	
	
    const String getParameterName (int index) override;
	float getParameter (int index) override;

	const String getParameterText (int index) override;
	float getParameterDefaultValue (int index) override;

    void setParameter (int index, float newValue) override;
	bool isParameterAutomatable (int idx) const override;
	
	// factory presets - a VST concept - TODO
    int getNumPrograms() { return 1; }
    int getCurrentProgram() { return 1; }
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
	void handleHubNotification(MLSymbol action, const MLProperty val);
#endif
	
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
    void saveStateToLongFileName(const std::string& path);
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
	const MLFileCollection& getScaleCollection() { return *(mScaleFiles); }
	const MLFileCollection& getPresetCollection() { return *(mPresetFiles); }
	
	// presets
	void clearPresetCollection() { mPresetFiles->clear(); }
	void searchForPresets() { mPresetFiles->processFilesImmediate(); }
	
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

	virtual MLPoint getDefaultEditorSize() = 0;
	
protected:
	
	void processMIDI (MidiBuffer& midiMessages, MLControlEventVector & events);

	// set what kind of event input we are listening to (MIDI or MIDI_MPE or OSC)
	void setInputProtocol(int p);
	
	// set the parameter of the Engine but not the Model property.
	void setParameterWithoutProperty (MLSymbol paramName, float newValue);
	void setStringParameterWithoutProperty (MLSymbol paramName, const std::string& newValue);
	void setSignalParameterWithoutProperty (MLSymbol paramName, const MLSignal& newValue);
		
	// Engine creates graphs of Processors, does the work
	MLDSPEngine	mEngine;	

	int mInputProtocol;
		
	typedef std::shared_ptr<XmlElement> XmlElementPtr;
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

	// MLTEST any reason for unique_ptr not just value here??
	std::unique_ptr<MLFileCollection> mScaleFiles;
    std::unique_ptr<MLFileCollection> mPresetFiles;
    std::unique_ptr<MLFileCollection> mMIDIProgramFiles;
	
	// saved state for editor
	MLRect mEditorRect;
	bool mEditorNumbersOn;
	bool mEditorAnimationsOn;
	
	bool mInitialized;
    
    // vector of control events to send to engine along with each block of audio.
    MLControlEventVector mControlEvents;
	
	std::unique_ptr<MLAppState> mpPatchState;
	std::unique_ptr<MLEnvironmentModel> mpEnvironmentModel;
	std::unique_ptr<MLAppState> mpEnvironmentState;
	std::unique_ptr<cJSON> mpDefaultEnvironmentState;
	
#if defined(__APPLE__)
	MLT3DHub mT3DHub;
#endif
	
#if OSC_PARAMS	
	// transmit sequencer state including OSC port offset on port 9123
	std::unique_ptr<UdpTransmitSocket> mSeqInfoSocket;
	std::vector<char> mpOSCBuf;
	void sendSeqInfo();
	int mVisSendCounter;
	MLProcList mSequencerList;
	
	
	// visuals out OSC
	std::unique_ptr<UdpTransmitSocket> mVisualsSocket;
	std::vector<char> mpOSCVisualsBuf;
	void sendVisuals();
	MLProcList mRMSProcList;
	
	// program out OSC
	void sendProgramChange(int pgm);
	
	
#endif // OSC_PARAMS
	

	
	void setSequence(const MLSignal& seq);

public:
	
};

#endif  // __PLUGINPROCESSOR__
