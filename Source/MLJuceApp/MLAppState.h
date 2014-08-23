
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
    public MLPropertyListener,
	public Timer
{
public:
	MLAppState(MLModel* , MLAppView* , const char* , const char* , int );
    ~MLAppState();
	
	void timerCallback();
	
	// save the application state to a global preferences location so it persists on relaunch.
	// TODO move the JSON stuff into propertySet::getStateAsJSON() or similar
    void saveState();

	void loadStateFromJSON(cJSON* pNode, int depth = 0);

	// load the application stats from the global preferences.
	bool loadSavedState();
	// load a state to use if there are no saved preferences. 
	void loadDefaultState();

	//const std::string& getStateAsText();
	void setStateFromText(const std::string& stateAsText);

	// MLPropertyListener interface
	void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);

protected:
	MLAppView* mpAppView;
	const char* mpMakerName;
	const char* mpAppName;
	int mVersion;
	
private:
	MLModel* mpModel;
	File getStateDir() const;
	File getStateFile() const;
	
};

#endif // __ML_APP_STATE_H
