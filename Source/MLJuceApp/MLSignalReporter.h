
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
	MLSignalView* addSignalViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr, int size, int priority = 0);
	
	// view all of the signals in the map.
	void viewSignals();

protected:
	void viewChangedSignals();	
	void viewAllSignals();
	int viewOneSignal(MLSymbol signalName, bool forceView, int priority = 0);
	void redrawSignals();
	
	typedef std::shared_ptr<MLSignalView> MLSignalViewPtr;
	typedef std::list<MLSignalViewPtr> MLSignalViewList;
	typedef std::map<MLSymbol, MLSignalViewList> MLSignalViewListMap;
	typedef std::map<MLSymbol, MLSignalPtr> MLSymbolToSignalMap;
	typedef std::map<MLSymbol, int> ViewPriorityMap;

	MLPluginProcessor* mpProcessor;
	MLSymbolToSignalMap mSignalBuffers;
	MLSymbolToSignalMap mSignalBuffers2;
    ViewPriorityMap mViewPriorityMap;
    
    // map of view lists
	MLSignalViewListMap mSignalViewsMap;

    int mViewIndex;
    std::vector<MLSymbol> mSignalNames;
	
	bool mNeedsRedraw;
};

#endif // __ML_SIGNAL_REPORTER_H