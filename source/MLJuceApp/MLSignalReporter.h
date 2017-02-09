
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_SIGNAL_REPORTER_H
#define __ML_SIGNAL_REPORTER_H

// MLSignalReporter is a mixin class used by objects like MLPluginController
// that display one or more published signals from the DSP engine. 

#include "MLPluginProcessor.h"
#include "MLSignalView.h"

#pragma mark MLSignalReporter 

class MLSignalReporter
{
public:
	MLSignalReporter(MLPluginProcessor* p);
    ~MLSignalReporter();
    
 	// add a signal view entry to the map and connect it to a new signal viewer.
	MLSignalView* addSignalViewToMap(ml::Symbol p, MLWidget* w, ml::Symbol attr, int size, int priority = 0, int frameSize = 1);
	
	// view all of the signals in the map.
	void viewSignals();

protected:
	void viewChangedSignals();	
	void viewAllSignals();
	int viewOneSignal(ml::Symbol signalName, bool forceView, int priority = 0);
	void redrawSignals();
	
	typedef std::shared_ptr<MLSignalView> MLSignalViewPtr;

	MLPluginProcessor* mpProcessor;
	std::map<ml::Symbol, MLSignalPtr> mSignalBuffers;
	std::map<ml::Symbol, MLSignalPtr> mSignalBuffers2;
    std::map<ml::Symbol, int> mViewPriorityMap;
    
    // map of view lists
	std::map<ml::Symbol, std::list<MLSignalViewPtr> > mSignalViewsMap;

    int mViewIndex;
    std::vector<ml::Symbol> mSignalNames;
	
	std::map<ml::Symbol, int> mVoicesBySignalName;
	
	bool mNeedsRedraw;
};

#endif // __ML_SIGNAL_REPORTER_H
