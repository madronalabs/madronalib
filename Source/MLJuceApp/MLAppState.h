
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
#include <tr1/memory>
#endif

extern const char* kMLStateDirName;

class MLAppState : 
    public MLPropertyListener,
	public Timer
{
public:
	MLAppState(MLModel*, const std::string&, const std::string&, const std::string&, int );
    ~MLAppState();
	
	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
	void timerCallback();
	
	// get and save state
	// TODO JUCE-free
	juce::MemoryBlock getStateAsBinary();
	String getStateAsText();
	void getStateAsJSON(cJSON* pNode);
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
	std::string mName;
	std::string mMakerName;
	std::string mAppName;
	int mAppVersion;
	
private:
	MLModel* mpModel;
	File getAppStateDir() const;
	File getAppStateFile() const;
	std::vector<juce::MemoryBlock> mStateStack;
};

typedef std::tr1::shared_ptr<MLAppState> MLAppStatePtr;

#endif // __ML_APP_STATE_H
