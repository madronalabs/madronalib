
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_APP_STATE_H
#define __ML_APP_STATE_H

#include "JuceHeader.h"
#include "MLModel.h"
#include "MLWidget.h"
#include "MLAppView.h"
#include "cJSON.h"

#ifdef _WIN32
#include <memory>
#else
#include <memory>
#endif

#include <set>

extern const char* kMLStateDirName;

class MLAppState : 
    public MLPropertyListener,
	public Timer
{
public:
	MLAppState(MLPropertySet*, const std::string& name, const std::string& makerName, 
	   const std::string& appName, int version);

    ~MLAppState();
	
	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	void timerCallback();
	
	void ignoreProperty(MLSymbol property);
	
	// get and save state
	// TODO JUCE-free
	juce::MemoryBlock getStateAsBinary();
	String getStateAsText();
	cJSON* getStateAsJSON();
	void saveStateToStateFile();

	// load and set state
	bool loadStateFromAppStateFile();
	void setStateFromBinary(const juce::MemoryBlock& newState);
	bool setStateFromText(String stateStr);
	void setStateFromJSON(cJSON* pNode, int depth = 0);

	// load a state to use if there are no saved preferences.
	void loadDefaultState();
	
	// state stack
	void clearStateStack();
	void pushStateToStack();
	void popStateFromStack();
	void returnToFirstSavedState();
	
protected:
	std::string mExtraName;
	std::string mMakerName;
	std::string mAppName;
	int mAppVersion;
	
private:
	MLPropertySet* mpTarget;
	File getAppStateDir() const;
	File getAppStateFile() const;
	std::vector<juce::MemoryBlock> mStateStack;
	std::set<MLSymbol> mIgnoredProperties;
};

#endif // __ML_APP_STATE_H
