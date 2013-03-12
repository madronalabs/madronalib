
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

extern const char* kMLStateDirName;

class MLAppState : 
	public MLModelListener,
	public Timer
{
public:
	MLAppState(MLModel* , MLAppView* , const char* , const char* , int );
    ~MLAppState();
	
	void timerCallback();
	
	// save the application stats to a global preferences location so it persists on relaunch.
	void saveState();

	void loadStateFromJSON(cJSON* pNode, int depth = 0);

	// load the application stats from the global preferences.
	bool loadSavedState();
	// load a state to use if there are no saved preferences. 
	void loadDefaultState();

	//const std::string& getStateAsText();
	void setStateFromText(const std::string& stateAsText);

	// MLModelListener interface
	void doParamChangeAction(MLSymbol param, const MLModelParam& oldVal, const MLModelParam& newVal);

protected:
	MLAppView* mpAppView;
	const char* mpMakerName;
	const char* mpAppName;
	int mVersion;
	
private:
	File getStateDir() const;
	File getStateFile() const;
	
};

#endif // __ML_APP_STATE_H
